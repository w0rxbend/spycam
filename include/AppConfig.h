#pragma once

#include <Arduino.h>
#include "SerialLog.h"
#include "esp_camera.h"

/*
  Performance and throttling guide

  If the ESP32 gets hot, resets, drops Wi-Fi, or cannot keep up with the stream,
  reduce the camera and network load in this order:

  1. Lower TARGET_FPS. This directly increases FRAME_INTERVAL_MS and sends fewer
     frames per second. Example: 8 -> 5 -> 3.
  2. Lower CAMERA_FRAME_SIZE. VGA is heavier than QVGA; smaller frames use less
     RAM, CPU, Wi-Fi bandwidth, and server bandwidth.
  3. Increase CAMERA_JPEG_QUALITY. In the ESP32 camera driver, lower numbers mean
     better quality/larger files; higher numbers mean lower quality/smaller files.
     Example: 14 -> 18 -> 22.
  4. Reduce CAMERA_FB_COUNT to 1 if memory pressure is more important than capture
     smoothness. Two buffers can improve throughput but consume more RAM.
  5. Keep CAMERA_XCLK_FREQ_HZ at 20 MHz if camera probing fails with
     ESP_ERR_NOT_FOUND. Some OV2640 modules will not initialize reliably with a
     lower external clock.
  6. Enable WIFI_SLEEP_ENABLED to reduce Wi-Fi power draw. This can add latency or
     lower throughput, so disable it again if streaming becomes unreliable.
  7. Keep the NO_PSRAM values conservative. Boards without PSRAM should use lower
     frame sizes and a single frame buffer to avoid allocation failures.

  The safest first change for throttling is usually TARGET_FPS. The safest first
  change for memory or Wi-Fi pressure is usually CAMERA_FRAME_SIZE.
*/

#if __has_include("credentials.h")
#include "credentials.h"
#endif

namespace app_config_credentials {

#ifdef WIFI_SSID
// Possible values: any Wi-Fi SSID string defined as WIFI_SSID in credentials.h.
// This is the network name the ESP32 will join.
constexpr const char *WIFI_SSID_VALUE = WIFI_SSID;
#else
// Possible values: any Wi-Fi SSID string.
// Placeholder used when credentials.h is missing; replace through credentials.h before flashing.
constexpr const char *WIFI_SSID_VALUE = "<SSID>";
#endif

#ifdef WIFI_PASSWORD
// Possible values: any Wi-Fi password string defined as WIFI_PASSWORD in credentials.h.
// This is the password for WIFI_SSID_VALUE.
constexpr const char *WIFI_PASSWORD_VALUE = WIFI_PASSWORD;
#else
// Possible values: any Wi-Fi password string, or an empty string for open networks.
// Placeholder used when credentials.h is missing; replace through credentials.h before flashing.
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

// Possible values: None, FlipV, FlipH, Rotate180.
// Selects the image orientation correction applied after camera initialization.
enum class CameraRotation : uint8_t {
  // Leave the camera image as captured.
  None      = 0,
  // Flip the image vertically.
  FlipV     = 1,
  // Flip the image horizontally.
  FlipH     = 2,
  // Rotate the image 180 degrees by flipping both axes.
  Rotate180 = 3,
};

// Possible values: any Wi-Fi SSID string.
// Loaded from credentials.h when present; used by WiFi.begin().
constexpr const char *WIFI_SSID = app_config_credentials::WIFI_SSID_VALUE;
// Possible values: any Wi-Fi password string, or an empty string for open networks.
// Loaded from credentials.h when present; used by WiFi.begin().
constexpr const char *WIFI_PASSWORD = app_config_credentials::WIFI_PASSWORD_VALUE;

// Possible values: IPv4 address like "192.168.1.200" or DNS name like "cam-server.local".
// The ESP32 opens a TCP connection to this host and sends JPEG frames to it.
constexpr const char *SERVER_HOST = "192.168.1.200";
// Possible values: 1-65535; use the TCP port your frame receiver listens on.
// Avoid ports already used by other services on the server.
constexpr uint16_t SERVER_PORT = 5000;
// Possible values: 0-4294967295.
// Sent with every frame so the server can distinguish multiple ESP32-CAM devices.
constexpr uint32_t CAMERA_ID = 3;

// Possible values: common rates such as 9600, 57600, 115200, 230400, 460800, 921600.
// Must match platformio.ini monitor_speed and your serial monitor setting.
constexpr uint32_t SERIAL_BAUD = 115200;
// Possible values: serial_log::Level::Error, Warn, Info, Debug.
// Lower-noise choices are Error/Warn; Debug prints the most detail and can slow serial output.
constexpr serial_log::Level LOG_LEVEL = serial_log::Level::Info;
// Possible values: milliseconds greater than 0; examples: 1000, 5000, 30000.
// Controls periodic status log frequency for camera and sender tasks.
constexpr uint32_t STATUS_LOG_INTERVAL_MS = 5000;

// Possible values include FRAMESIZE_QQVGA(160x120), QVGA(320x240), VGA(640x480),
// SVGA(800x600), XGA(1024x768), SXGA(1280x1024), UXGA(1600x1200).
// Used when PSRAM is available; lower sizes reduce RAM, CPU, Wi-Fi load, and brownout risk.
constexpr framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;
// Possible values: usually 0-63 in the ESP32 camera driver.
// Lower numbers mean better image quality and larger frames; higher numbers reduce size and load.
constexpr int CAMERA_JPEG_QUALITY = 14;
// Possible values: usually 1 or 2 for ESP32-CAM.
// 1 uses less RAM/current; 2 can improve capture smoothness but increases memory pressure.
constexpr int CAMERA_FB_COUNT = 2;
// Possible values: same FRAMESIZE_* values as CAMERA_FRAME_SIZE.
// Used when PSRAM is not available; keep this conservative because frames are stored in internal RAM.
constexpr framesize_t CAMERA_FRAME_SIZE_NO_PSRAM = FRAMESIZE_QVGA;
// Possible values: usually 1 for boards without PSRAM.
// Higher values can fail allocation or destabilize the board when only internal RAM is available.
constexpr int CAMERA_FB_COUNT_NO_PSRAM = 1;
// Possible values: commonly 20000000; sometimes 10000000 works on specific modules.
// 20 MHz is the most reliable OV2640 probe value; lower values can cause ESP_ERR_NOT_FOUND.
constexpr int CAMERA_XCLK_FREQ_HZ = 20000000;
// Possible values: CameraRotation::None, FlipV, FlipH, Rotate180.
// Use this to correct an upside-down or mirrored mounted camera without changing server code.
constexpr CameraRotation CAMERA_ROTATION = CameraRotation::None;

// Possible values: positive FPS values; do not set to 0 because FRAME_INTERVAL_MS divides by it.
// Lower values reduce heat, current spikes, Wi-Fi bandwidth, and server load.
constexpr uint32_t TARGET_FPS = 4;
// Possible values: derived automatically from TARGET_FPS; do not edit directly unless needed.
// Milliseconds between capture attempts, calculated as 1000 / TARGET_FPS.
constexpr uint32_t FRAME_INTERVAL_MS = 1000 / TARGET_FPS;

// Possible values: true or false.
// true enables Wi-Fi modem sleep to reduce power draw; false favors lower latency and throughput stability.
constexpr bool WIFI_SLEEP_ENABLED = true;

// Possible values: milliseconds greater than 0; examples: 5000, 15000, 30000.
// Maximum time to wait for Wi-Fi association before backing off and retrying.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
// Possible values: milliseconds greater than 0; examples: 3000, 8000, 15000.
// Maximum time to wait while opening the TCP connection to SERVER_HOST:SERVER_PORT.
constexpr uint32_t TCP_CONNECT_TIMEOUT_MS = 8000;
// Possible values: milliseconds greater than 0; examples: 5000, 10000, 30000.
// Maximum time allowed for sending one full frame before the connection is dropped and retried.
constexpr uint32_t SEND_TIMEOUT_MS = 10000;
// Possible values: milliseconds greater than 0 and less than or equal to RECONNECT_BACKOFF_MAX_MS.
// Initial delay after Wi-Fi or TCP failure; short values retry faster but use more power.
constexpr uint32_t RECONNECT_BACKOFF_MIN_MS = 500;
// Possible values: milliseconds greater than or equal to RECONNECT_BACKOFF_MIN_MS.
// Maximum retry delay after repeated failures; longer values reduce retry spam and power use.
constexpr uint32_t RECONNECT_BACKOFF_MAX_MS = 30000;

// Possible values: task stack sizes in bytes; examples: 4096, 6144, 8192.
// Increase if the camera task overflows; decrease only if memory is tight and testing is stable.
constexpr uint32_t CAMERA_TASK_STACK = 6144;
// Possible values: task stack sizes in bytes; examples: 4096, 8192, 12288.
// Network/TCP code often needs more stack than simple tasks; increase if sender crashes unexpectedly.
constexpr uint32_t SENDER_TASK_STACK = 8192;
// Possible values: 0 to configMAX_PRIORITIES - 1; higher values run before lower-priority tasks.
// Keep above the sender if capture timing is more important than immediate network sends.
constexpr UBaseType_t CAMERA_TASK_PRIORITY = 2;
// Possible values: 0 to configMAX_PRIORITIES - 1; higher values run before lower-priority tasks.
// Kept below CAMERA_TASK_PRIORITY so capture stays responsive while sending happens in the background.
constexpr UBaseType_t SENDER_TASK_PRIORITY = 1;

} // namespace app_config
