#pragma once

#include <Arduino.h>
#include "esp_camera.h"

class LatestFrameSlot {
public:
  LatestFrameSlot();
  bool begin();
  void put(camera_fb_t *frame);
  camera_fb_t *takeLatest(TickType_t waitTicks);

private:
  SemaphoreHandle_t mutex_;
  SemaphoreHandle_t frameReady_;
  camera_fb_t *frame_;
};
