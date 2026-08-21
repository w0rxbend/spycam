#pragma once

#include <Arduino.h>

#include "CameraSettings.h"
#include "SenderSettings.h"
#include "SerialLog.h"

/*
  Every tunable value for this firmware, in one place.

  This header is the composition root's data: nothing here is read by
  CameraManager or TcpFrameSender directly. main.cpp packs these constants into
  camera::Settings and sender::Settings and hands them to the objects that need
  them, so each class states its own inputs and neither depends on this file.

  Performance and throttling guide

  If the ESP32 gets hot, resets, drops Wi-Fi, or cannot keep up with the stream,
  reduce the camera and network load in this order:

  1. Lower TARGET_FPS. This increases FRAME_INTERVAL_MS and sends fewer frames
     per second. Example: 8 -> 5 -> 3.
  2. Lower CAMERA_FRAME_SIZE. VGA is heavier than QVGA; smaller frames use less
     RAM, CPU, Wi-Fi bandwidth, and server bandwidth.
  3. Increase CAMERA_JPEG_QUALITY. The driver's scale is inverted: lower numbers
     mean better quality and larger files. Example: 14 -> 18 -> 22.
  4. Reduce CAMERA_FB_COUNT to 1 if memory pressure matters more than capture
     smoothness. Two buffers improve throughput but consume more RAM.
  5. Keep CAMERA_XCLK_FREQ_HZ at 20 MHz if camera probing fails with
     ESP_ERR_NOT_FOUND. Some OV2640 modules will not initialize reliably with a
     lower external clock.
  6. Leave WIFI_SLEEP_ENABLED off while streaming. Enable it only if power draw
     matters more than latency and throughput.
  7. Keep the NO_PSRAM values conservative. Boards without PSRAM should use lower
     frame sizes and a single frame buffer to avoid allocation failures.

  The safest first change for throttling is usually TARGET_FPS. The safest first
  change for memory or Wi-Fi pressure is usually CAMERA_FRAME_SIZE.
*/

#if __has_include("credentials.h")
#include "credentials.h"
#endif

// credentials.h is gitignored and defines WIFI_SSID and WIFI_PASSWORD as macros.
// They are captured into constants here and then undefined, so the rest of the
// codebase sees ordinary typed values instead of preprocessor symbols that could
// collide with anything else named SSID or PASSWORD.
namespace app_config_credentials {

#ifdef WIFI_SSID
constexpr const char *WIFI_SSID_VALUE = WIFI_SSID;
#else
// Placeholder used when credentials.h is missing. The board will not associate
// until credentials.h supplies a real network name.
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

constexpr const char *WIFI_SSID = app_config_credentials::WIFI_SSID_VALUE;
constexpr const char *WIFI_PASSWORD = app_config_credentials::WIFI_PASSWORD_VALUE;

// IPv4 literal like "192.168.1.200" or a DNS name like "cam-server.local".
constexpr const char *SERVER_HOST = "192.168.1.200";
constexpr uint16_t SERVER_PORT = 5000;
// Sent with every frame so one server can distinguish multiple ESP32-CAM boards.
constexpr uint32_t CAMERA_ID = 3;

// Must match monitor_speed in platformio.ini.
constexpr uint32_t SERIAL_BAUD = 115200;
// Error and Warn are the quiet choices; Debug logs every frame and can itself
// slow the stream down.
constexpr serial_log::Level LOG_LEVEL = serial_log::Level::Info;
// How often the camera and sender tasks print their running counters.
constexpr uint32_t STATUS_LOG_INTERVAL_MS = 5000;

// FRAMESIZE_QQVGA(160x120), QVGA(320x240), VGA(640x480), SVGA(800x600),
// XGA(1024x768), SXGA(1280x1024), UXGA(1600x1200).
constexpr framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;
// Inverted scale, usually 0-63: lower means better quality and larger frames.
constexpr int CAMERA_JPEG_QUALITY = 14;
constexpr int CAMERA_FB_COUNT = 2;
// Used when no PSRAM is fitted; frames then live in internal RAM, so keep these
// conservative or allocation fails.
constexpr framesize_t CAMERA_FRAME_SIZE_NO_PSRAM = FRAMESIZE_QVGA;
constexpr int CAMERA_FB_COUNT_NO_PSRAM = 1;
// 20 MHz is the most reliable OV2640 probe value; lower values can cause
// ESP_ERR_NOT_FOUND on some modules.
constexpr int CAMERA_XCLK_FREQ_HZ = 20000000;
constexpr camera::Rotation CAMERA_ROTATION = camera::Rotation::None;

// Must be greater than zero: FRAME_INTERVAL_MS divides by it.
constexpr uint32_t TARGET_FPS = 4;
static_assert(TARGET_FPS > 0, "TARGET_FPS must be greater than zero");
// Derived from TARGET_FPS; the capture loop's target period.
constexpr uint32_t FRAME_INTERVAL_MS = 1000 / TARGET_FPS;

// Modem sleep is off by default because it adds latency and destabilizes
// continuous JPEG streaming on this board. Set to true to trade streaming
// stability for lower Wi-Fi power draw.
constexpr bool WIFI_SLEEP_ENABLED = false;

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t TCP_CONNECT_TIMEOUT_MS = 8000;
// Budget for sending one whole frame before the connection is dropped and retried.
constexpr uint32_t SEND_TIMEOUT_MS = 10000;
// Retry delay after a failure, doubling from min up to max.
constexpr uint32_t RECONNECT_BACKOFF_MIN_MS = 500;
constexpr uint32_t RECONNECT_BACKOFF_MAX_MS = 30000;
static_assert(RECONNECT_BACKOFF_MIN_MS <= RECONNECT_BACKOFF_MAX_MS,
              "Reconnect backoff minimum must not exceed the maximum");

// Stack sizes in bytes. Network code needs more stack than the capture loop.
constexpr uint32_t CAMERA_TASK_STACK = 6144;
constexpr uint32_t SENDER_TASK_STACK = 8192;
// The camera runs above the sender so capture timing stays responsive while
// sending happens in the background.
constexpr UBaseType_t CAMERA_TASK_PRIORITY = 2;
constexpr UBaseType_t SENDER_TASK_PRIORITY = 1;
static_assert(CAMERA_TASK_PRIORITY > SENDER_TASK_PRIORITY,
              "The camera task must outrank the sender task");

// Packs the constants above into the settings each object is constructed with.
constexpr camera::Settings cameraSettings()
{
  return camera::Settings{
      CAMERA_FRAME_SIZE,
      CAMERA_FRAME_SIZE_NO_PSRAM,
      CAMERA_JPEG_QUALITY,
      CAMERA_FB_COUNT,
      CAMERA_FB_COUNT_NO_PSRAM,
      CAMERA_XCLK_FREQ_HZ,
      CAMERA_ROTATION,
  };
}

constexpr sender::Settings senderSettings()
{
  return sender::Settings{
      WIFI_SSID,
      WIFI_PASSWORD,
      WIFI_SLEEP_ENABLED,
      SERVER_HOST,
      SERVER_PORT,
      CAMERA_ID,
      WIFI_CONNECT_TIMEOUT_MS,
      TCP_CONNECT_TIMEOUT_MS,
      SEND_TIMEOUT_MS,
      RECONNECT_BACKOFF_MIN_MS,
      RECONNECT_BACKOFF_MAX_MS,
      STATUS_LOG_INTERVAL_MS,
      FRAME_INTERVAL_MS,
  };
}

} // namespace app_config
