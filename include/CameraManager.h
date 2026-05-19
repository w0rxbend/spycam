#pragma once

#include <Arduino.h>
#include "esp_camera.h"

class CameraManager {
public:
  bool begin();
  camera_fb_t *capture();
  void release(camera_fb_t *frame);
};
