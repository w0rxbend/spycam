#include <Arduino.h>

#include "AppConfig.h"
#include "CameraManager.h"
#include "LatestFrameSlot.h"
#include "SerialLog.h"
#include "TcpFrameSender.h"

namespace
{

  // Composition root: the only place that reads AppConfig.h and the only place
  // that decides which concrete objects exist.
  CameraManager cameraManager(app_config::cameraSettings());
  LatestFrameSlot latestFrame;
  TcpFrameSender frameSender(app_config::senderSettings());

  void cameraTask(void *parameter)
  {
    static_cast<void>(parameter);
    TickType_t lastWake = xTaskGetTickCount();
    uint32_t capturedFrames = 0;
    uint32_t captureFailures = 0;
    uint32_t lastStatusAt = millis();

    for (;;)
    {
      camera_fb_t *frame = cameraManager.capture();
      if (frame == nullptr)
      {
        ++captureFailures;
        serial_log::warn("Camera capture failed");
        vTaskDelay(pdMS_TO_TICKS(250));
        lastWake = xTaskGetTickCount();
        continue;
      }

      ++capturedFrames;
      latestFrame.put(frame);

      const uint32_t now = millis();
      if (now - lastStatusAt >= app_config::STATUS_LOG_INTERVAL_MS)
      {
        serial_log::info("Camera task: captured=%lu capture_failures=%lu",
                         static_cast<unsigned long>(capturedFrames),
                         static_cast<unsigned long>(captureFailures));
        lastStatusAt = now;
      }

      if (xTaskDelayUntil(&lastWake, pdMS_TO_TICKS(app_config::FRAME_INTERVAL_MS)) == pdFALSE)
      {
        // The capture overran the period. Drop the backlog rather than
        // free-running at full rate until the deadline catches up.
        lastWake = xTaskGetTickCount();
      }
    }
  }

  void senderTask(void *parameter)
  {
    static_cast<void>(parameter);
    frameSender.begin();

    for (;;)
    {
      if (!frameSender.ensureConnected())
      {
        continue;
      }

      camera_fb_t *frame = latestFrame.takeLatest(pdMS_TO_TICKS(1000));
      if (frame == nullptr)
      {
        continue;
      }

      frameSender.sendFrame(frame);
      cameraManager.release(frame);
    }
  }

} // namespace

void setup()
{
  serial_log::begin(app_config::SERIAL_BAUD, app_config::LOG_LEVEL);
  // The AI-Thinker board has no USB-CDC; a host terminal reattaching after
  // reset can miss the first bytes. One short settle beats losing the banner.
  delay(500);
  serial_log::info("ESP32-CAM TCP JPEG client starting");
  serial_log::info("Target: tcp://%s:%u camera_id=%lu target_fps=%lu",
                   app_config::SERVER_HOST,
                   app_config::SERVER_PORT,
                   static_cast<unsigned long>(app_config::CAMERA_ID),
                   static_cast<unsigned long>(app_config::TARGET_FPS));

  if (!latestFrame.begin())
  {
    serial_log::error("Failed to create frame slot synchronization primitives");
    delay(1000);
    ESP.restart();
  }

  if (!cameraManager.begin())
  {
    serial_log::error("Failed to initialize camera");
    delay(1000);
    ESP.restart();
  }

  BaseType_t taskCreated = xTaskCreatePinnedToCore(cameraTask,
                                                   "camera",
                                                   app_config::CAMERA_TASK_STACK,
                                                   nullptr,
                                                   app_config::CAMERA_TASK_PRIORITY,
                                                   nullptr,
                                                   1);
  if (taskCreated != pdPASS)
  {
    serial_log::error("Failed to create camera task");
    delay(1000);
    ESP.restart();
  }
  serial_log::info("Camera task started");

  taskCreated = xTaskCreatePinnedToCore(senderTask,
                                        "tcp_sender",
                                        app_config::SENDER_TASK_STACK,
                                        nullptr,
                                        app_config::SENDER_TASK_PRIORITY,
                                        nullptr,
                                        0);
  if (taskCreated != pdPASS)
  {
    serial_log::error("Failed to create TCP sender task");
    delay(1000);
    ESP.restart();
  }
  serial_log::info("TCP sender task started");
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
