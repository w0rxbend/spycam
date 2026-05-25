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
  5. Keep the NO_PSRAM values conservative. Boards without PSRAM should use lower
     frame sizes and a single frame buffer to avoid allocation failures.

  The safest first change for throttling is usually TARGET_FPS. The safest first
  change for memory or Wi-Fi pressure is usually CAMERA_FRAME_SIZE.
*/

#if __has_include("credentials.h")
#include "credentials.h"
#endif

namespace app_config_credentials {

#ifdef WIFI_SSID
// Wi-Fi network name loaded from include/credentials.h when provided.
constexpr const char *WIFI_SSID_VALUE = WIFI_SSID;
#else
// Placeholder Wi-Fi network name used when include/credentials.h is missing.
constexpr const char *WIFI_SSID_VALUE = "<SSID>";
#endif

#ifdef WIFI_PASSWORD
// Wi-Fi password loaded from include/credentials.h when provided.
constexpr const char *WIFI_PASSWORD_VALUE = WIFI_PASSWORD;
#else
// Placeholder Wi-Fi password used when include/credentials.h is missing.
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

// Camera image orientation to apply after capture.
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

// Wi-Fi network name used by the device.
constexpr const char *WIFI_SSID = app_config_credentials::WIFI_SSID_VALUE;
// Wi-Fi password used by the device.
constexpr const char *WIFI_PASSWORD = app_config_credentials::WIFI_PASSWORD_VALUE;

// IP address or hostname of the server that receives camera frames.
constexpr const char *SERVER_HOST = "192.168.1.200";
// TCP port on the server that receives camera frames.
constexpr uint16_t SERVER_PORT = 5000;
// Numeric camera identifier sent to the server so multiple cameras can be distinguished.
constexpr uint32_t CAMERA_ID = 3;

// Serial monitor baud rate for logs and diagnostics.
constexpr uint32_t SERIAL_BAUD = 115200;
// Minimum log level printed to the serial monitor.
constexpr serial_log::Level LOG_LEVEL = serial_log::Level::Info;
// How often runtime status messages are printed.
constexpr uint32_t STATUS_LOG_INTERVAL_MS = 5000;

// Camera resolution used on boards with PSRAM. Lower this to reduce CPU, RAM, and Wi-Fi load.
constexpr framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;
// JPEG compression level. Lower numbers produce better quality/larger frames; higher numbers reduce size and load.
constexpr int CAMERA_JPEG_QUALITY = 14;
// Number of frame buffers with PSRAM. More buffers can improve smoothness but use more memory.
constexpr int CAMERA_FB_COUNT = 2;
// Fallback camera resolution for boards without PSRAM.
constexpr framesize_t CAMERA_FRAME_SIZE_NO_PSRAM = FRAMESIZE_QVGA;
// Fallback frame buffer count for boards without PSRAM.
constexpr int CAMERA_FB_COUNT_NO_PSRAM = 1;
// Camera image rotation or flip correction.
constexpr CameraRotation CAMERA_ROTATION = CameraRotation::None;

// Target number of frames captured and sent per second. Lower this first if the ESP32 throttles.
constexpr uint32_t TARGET_FPS = 8;
// Delay between frame attempts, derived from TARGET_FPS.
constexpr uint32_t FRAME_INTERVAL_MS = 1000 / TARGET_FPS;

// Maximum time to wait for Wi-Fi connection before retrying or reporting failure.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
// Maximum time to wait while opening a TCP connection to the server.
constexpr uint32_t TCP_CONNECT_TIMEOUT_MS = 8000;
// Maximum time allowed for sending one frame to the server.
constexpr uint32_t SEND_TIMEOUT_MS = 10000;
// Initial reconnect delay after a connection failure.
constexpr uint32_t RECONNECT_BACKOFF_MIN_MS = 500;
// Maximum reconnect delay after repeated connection failures.
constexpr uint32_t RECONNECT_BACKOFF_MAX_MS = 30000;

// FreeRTOS stack size for the camera capture task.
constexpr uint32_t CAMERA_TASK_STACK = 6144;
// FreeRTOS stack size for the network sender task.
constexpr uint32_t SENDER_TASK_STACK = 8192;
// FreeRTOS priority for the camera capture task. Higher values run before lower-priority tasks.
constexpr UBaseType_t CAMERA_TASK_PRIORITY = 2;
// FreeRTOS priority for the network sender task. Lower than camera capture so capture stays responsive.
constexpr UBaseType_t SENDER_TASK_PRIORITY = 1;

} // namespace app_config
