#pragma once

#include <Arduino.h>
#include "SerialLog.h"
#include "esp_camera.h"

#if __has_include("credentials.h")
#include "credentials.h"
#endif

namespace app_config_credentials {

#ifdef WIFI_SSID
constexpr const char *WIFI_SSID_VALUE = WIFI_SSID;
#else
constexpr const char *WIFI_SSID_VALUE = "<SSID>";
#endif

#ifdef WIFI_PASSWORD
constexpr const char *WIFI_PASSWORD_VALUE = WIFI_PASSWORD;
#else
constexpr const char *WIFI_PASSWORD_VALUE = "<PASSWORD>";
#endif

} // namespace app_config_credentials

#ifdef WIFI_SSID
#undef WIFI_SSID
#endif

#ifdef WIFI_PASSWORD
#undef WIFI_PASSWORD
#endif

namespace app_config {

enum class CameraRotation : uint8_t {
  None      = 0,
  FlipV     = 1,
  FlipH     = 2,
  Rotate180 = 3,
};

constexpr const char *WIFI_SSID = app_config_credentials::WIFI_SSID_VALUE;
constexpr const char *WIFI_PASSWORD = app_config_credentials::WIFI_PASSWORD_VALUE;

constexpr const char *SERVER_HOST = "192.168.1.200";
constexpr uint16_t SERVER_PORT = 5000;
constexpr uint32_t CAMERA_ID = 5;

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr serial_log::Level LOG_LEVEL = serial_log::Level::Info;
constexpr uint32_t STATUS_LOG_INTERVAL_MS = 5000;

constexpr framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;
constexpr int CAMERA_JPEG_QUALITY = 12;
constexpr int CAMERA_FB_COUNT = 2;
constexpr framesize_t CAMERA_FRAME_SIZE_NO_PSRAM = FRAMESIZE_QVGA;
constexpr int CAMERA_FB_COUNT_NO_PSRAM = 1;
constexpr CameraRotation CAMERA_ROTATION = CameraRotation::Rotate180;

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
