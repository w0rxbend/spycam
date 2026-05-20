#include <Arduino.h>

#include "AppConfig.h"
#include "CameraManager.h"
#include "LatestFrameSlot.h"
#include "TcpFrameSender.h"

namespace {

CameraManager camera;
LatestFrameSlot latestFrame;
TcpFrameSender sender(app_config::SERVER_HOST, app_config::SERVER_PORT);

void cameraTask(void *parameter)
{
  static_cast<void>(parameter);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    camera_fb_t *frame = camera.capture();
    if (frame == nullptr) {
      Serial.println("Camera capture failed");
      vTaskDelay(pdMS_TO_TICKS(250));
      lastWake = xTaskGetTickCount();
      continue;
    }

    latestFrame.put(frame);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(app_config::FRAME_INTERVAL_MS));
  }
}

void senderTask(void *parameter)
{
  static_cast<void>(parameter);
  sender.begin();

  for (;;) {
    if (!sender.ensureConnected()) {
      continue;
    }

    camera_fb_t *frame = latestFrame.takeLatest(pdMS_TO_TICKS(1000));
    if (frame == nullptr) {
      continue;
    }

    sender.sendFrame(frame);
    camera.release(frame);
  }
}

} // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("ESP32-CAM TCP JPEG client starting");

  if (!latestFrame.begin()) {
    Serial.println("Failed to create frame slot synchronization primitives");
    delay(1000);
    ESP.restart();
  }

  if (!camera.begin()) {
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
  if (taskCreated != pdPASS) {
    Serial.println("Failed to create camera task");
    delay(1000);
    ESP.restart();
  }

  taskCreated = xTaskCreatePinnedToCore(senderTask,
                                        "tcp_sender",
                                        app_config::SENDER_TASK_STACK,
                                        nullptr,
                                        app_config::SENDER_TASK_PRIORITY,
                                        nullptr,
                                        0);
  if (taskCreated != pdPASS) {
    Serial.println("Failed to create TCP sender task");
    delay(1000);
    ESP.restart();
  }
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
