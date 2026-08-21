#pragma once

#include <Arduino.h>
#include "esp_camera.h"

#include "CameraSettings.h"

// Owns the ESP32 camera driver: initializes the sensor and hands out frame
// buffers. Every buffer returned by capture() must be given back with release().
class CameraManager {
public:
  explicit CameraManager(const camera::Settings &settings);

  bool begin();
  camera_fb_t *capture();
  void release(camera_fb_t *frame);

private:
  camera::Settings settings_;
};
