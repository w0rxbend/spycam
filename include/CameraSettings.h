#pragma once

#include <stdint.h>

#include "esp_camera.h"

namespace camera {

// Image orientation correction applied after the sensor is initialized. Use this
// to fix an upside-down or mirrored physical mount without changing server code.
enum class Rotation : uint8_t {
  None      = 0,
  FlipV     = 1,
  FlipH     = 2,
  Rotate180 = 3,
};

// Everything CameraManager needs, handed to it at construction.
//
// The withPsram/noPsram pairs exist because the board is only sometimes fitted
// with PSRAM, and frames land in internal RAM when it is absent. CameraManager
// probes for PSRAM at begin() and picks the matching pair; callers supply both
// rather than deciding which applies.
struct Settings {
  framesize_t frameSize;
  framesize_t frameSizeNoPsram;
  // In the ESP32 camera driver this scale is inverted: LOWER numbers mean
  // better quality and larger frames. Usually 0-63.
  int jpegQuality;
  int fbCount;
  int fbCountNoPsram;
  // 20 MHz is the most reliable OV2640 probe value. Lower values make some
  // modules fail to initialize with ESP_ERR_NOT_FOUND.
  int xclkFreqHz;
  Rotation rotation;
};

} // namespace camera
