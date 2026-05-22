#include "LatestFrameSlot.h"

#include "SerialLog.h"

LatestFrameSlot::LatestFrameSlot() : mutex_(nullptr), frameReady_(nullptr), frame_(nullptr)
{
}

bool LatestFrameSlot::begin()
{
  mutex_ = xSemaphoreCreateMutex();
  frameReady_ = xSemaphoreCreateBinary();
  return mutex_ != nullptr && frameReady_ != nullptr;
}

void LatestFrameSlot::put(camera_fb_t *frame)
{
  if (frame == nullptr || mutex_ == nullptr || frameReady_ == nullptr) {
    if (frame != nullptr) {
      serial_log::warn("Dropping frame because latest-frame slot is not initialized");
      esp_camera_fb_return(frame);
    }
    return;
  }

  camera_fb_t *staleFrame = nullptr;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  staleFrame = frame_;
  frame_ = frame;
  xSemaphoreGive(mutex_);

  if (staleFrame != nullptr) {
    serial_log::debug("Dropping stale frame before sender consumed it");
    esp_camera_fb_return(staleFrame);
  }

  xSemaphoreGive(frameReady_);
}

camera_fb_t *LatestFrameSlot::takeLatest(TickType_t waitTicks)
{
  if (mutex_ == nullptr || frameReady_ == nullptr) {
    return nullptr;
  }

  const TickType_t startedAt = xTaskGetTickCount();

  for (;;) {
    const TickType_t elapsed = xTaskGetTickCount() - startedAt;
    if (elapsed >= waitTicks) {
      return nullptr;
    }

    if (xSemaphoreTake(frameReady_, waitTicks - elapsed) != pdTRUE) {
      return nullptr;
    }

    camera_fb_t *frame = nullptr;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    frame = frame_;
    frame_ = nullptr;
    xSemaphoreGive(mutex_);

    if (frame != nullptr) {
      return frame;
    }
  }
}
