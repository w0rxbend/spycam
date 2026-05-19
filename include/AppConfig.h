#pragma once

#include <Arduino.h>
#include "esp_camera.h"

namespace app_config {

constexpr const char *WIFI_SSID = "<SSID>";
constexpr const char *WIFI_PASSWORD = "<PASSWORD>";

constexpr const char *SERVER_HOST = "192.168.1.187";
constexpr uint16_t SERVER_PORT = 5000;

constexpr framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;
constexpr int CAMERA_JPEG_QUALITY = 12;
constexpr int CAMERA_FB_COUNT = 2;
constexpr framesize_t CAMERA_FRAME_SIZE_NO_PSRAM = FRAMESIZE_QVGA;
constexpr int CAMERA_FB_COUNT_NO_PSRAM = 1;

constexpr uint32_t TARGET_FPS = 8;
constexpr uint32_t FRAME_INTERVAL_MS = 1000 / TARGET_FPS;

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t TCP_CONNECT_TIMEOUT_MS = 8000;
constexpr uint32_t SEND_TIMEOUT_MS = 10000;
constexpr uint32_t RECONNECT_BACKOFF_MIN_MS = 500;
constexpr uint32_t RECONNECT_BACKOFF_MAX_MS = 30000;

constexpr uint32_t CAMERA_TASK_STACK = 6144;
constexpr uint32_t SENDER_TASK_STACK = 8192;
constexpr UBaseType_t CAMERA_TASK_PRIORITY = 2;
constexpr UBaseType_t SENDER_TASK_PRIORITY = 1;

} // namespace app_config
