#include "TcpFrameSender.h"

#include "AppConfig.h"
#include "FrameProtocol.h"
#include "SerialLog.h"

namespace {
constexpr size_t kSendChunkSize = 4096;
}

TcpFrameSender::TcpFrameSender(const char *host, uint16_t port)
    : host_(host),
      port_(port),
      sequence_(0),
      backoffMs_(app_config::RECONNECT_BACKOFF_MIN_MS),
      sentFrames_(0),
      failedSends_(0),
      lastStatusAt_(0)
{
}

void TcpFrameSender::begin()
{
  serial_log::info("Initializing WiFi/TCP sender");
  WiFi.mode(WIFI_STA);
  // WiFi.setSleep(app_config::WIFI_SLEEP_ENABLED);
  WiFi.setSleep(false);
  WiFi.persistent(false);
  client_.setNoDelay(true);
  client_.setTimeout(app_config::SEND_TIMEOUT_MS / 1000);
  lastStatusAt_ = millis();
}

bool TcpFrameSender::ensureConnected()
{
  if (!ensureWifiConnected()) {
    return false;
  }
  return ensureTcpConnected();
}

bool TcpFrameSender::sendFrame(camera_fb_t *frame)
{
  if (frame == nullptr || frame->buf == nullptr || frame->len == 0) {
    return false;
  }

  // Non-blocking precondition only: never sleep in a reconnect backoff here,
  // because the caller is holding a camera frame buffer while we run. The
  // sender task's own ensureConnected() does the waiting, at a point where no
  // buffer is checked out.
  if (WiFi.status() != WL_CONNECTED || !client_.connected()) {
    disconnect();
    return false;
  }

  uint8_t header[frame_protocol::HEADER_SIZE];
  frame_protocol::buildHeader(header, sequence_++, static_cast<uint32_t>(frame->len), millis());

  uint8_t cameraId[frame_protocol::CAMERA_ID_SIZE];
  frame_protocol::buildCameraId(cameraId, app_config::CAMERA_ID);

  const uint32_t startedAt = millis();
  const bool sent = sendAll(header, sizeof(header)) &&
                    sendAll(cameraId, sizeof(cameraId)) &&
                    sendAll(frame->buf, frame->len);
  const uint32_t elapsed = millis() - startedAt;

  if (!sent) {
    ++failedSends_;
    serial_log::warn("TCP send failed; connection will be reopened");
    disconnect();
    return false;
  }

  ++sentFrames_;
  resetBackoff();
  serial_log::debug("Sent frame camera=%lu seq=%lu bytes=%u elapsed=%lums",
                    static_cast<unsigned long>(app_config::CAMERA_ID),
                    static_cast<unsigned long>(sequence_ - 1),
                    static_cast<unsigned>(frame->len),
                    static_cast<unsigned long>(elapsed));

  const uint32_t now = millis();
  if (now - lastStatusAt_ >= app_config::STATUS_LOG_INTERVAL_MS) {
    serial_log::info("Sender task: sent=%lu failed_sends=%lu last_seq=%lu wifi_rssi=%ld",
                     static_cast<unsigned long>(sentFrames_),
                     static_cast<unsigned long>(failedSends_),
                     static_cast<unsigned long>(sequence_ - 1),
                     static_cast<long>(WiFi.RSSI()));
    lastStatusAt_ = now;
  }

  if (elapsed > app_config::FRAME_INTERVAL_MS) {
    serial_log::warn("Sender is slower than capture interval (%lums > %lums); stale frames will be dropped",
                     static_cast<unsigned long>(elapsed),
                     static_cast<unsigned long>(app_config::FRAME_INTERVAL_MS));
  }

  return true;
}

void TcpFrameSender::disconnect()
{
  if (client_.connected()) {
    client_.stop();
  }
}

bool TcpFrameSender::ensureWifiConnected()
{
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  disconnect();
  serial_log::info("Connecting WiFi");
  WiFi.disconnect(false);
  WiFi.begin(app_config::WIFI_SSID, app_config::WIFI_PASSWORD);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < app_config::WIFI_CONNECT_TIMEOUT_MS) {
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  if (WiFi.status() != WL_CONNECTED) {
    serial_log::warn("WiFi connect timed out");
    waitBackoff();
    return false;
  }

  serial_log::info("WiFi connected: ip=%s mac=%s rssi=%ld",
                   WiFi.localIP().toString().c_str(),
                   WiFi.macAddress().c_str(),
                   static_cast<long>(WiFi.RSSI()));
  resetBackoff();
  return true;
}

bool TcpFrameSender::ensureTcpConnected()
{
  if (client_.connected()) {
    return true;
  }

  client_.stop();
  serial_log::info("Connecting TCP %s:%u", host_, port_);
  const bool connected = client_.connect(host_, port_, app_config::TCP_CONNECT_TIMEOUT_MS);
  if (!connected) {
    serial_log::warn("TCP connect failed");
    waitBackoff();
    return false;
  }

  serial_log::info("TCP connected");
  resetBackoff();
  return true;
}

bool TcpFrameSender::sendAll(const uint8_t *data, size_t len)
{
  size_t sent = 0;
  const uint32_t startedAt = millis();

  while (sent < len) {
    if (!client_.connected()) {
      return false;
    }

    if (millis() - startedAt > app_config::SEND_TIMEOUT_MS) {
      serial_log::warn("TCP send timed out");
      return false;
    }

    const size_t remaining = len - sent;
    const size_t chunkLen = remaining > kSendChunkSize ? kSendChunkSize : remaining;
    const size_t written = client_.write(data + sent, chunkLen);
    if (written == 0) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    sent += written;
  }

  return true;
}

void TcpFrameSender::waitBackoff()
{
  const uint32_t delayMs = backoffMs_;
  backoffMs_ = min(backoffMs_ * 2, app_config::RECONNECT_BACKOFF_MAX_MS);
  serial_log::info("Reconnect backoff %lums", static_cast<unsigned long>(delayMs));
  vTaskDelay(pdMS_TO_TICKS(delayMs));
}

void TcpFrameSender::resetBackoff()
{
  backoffMs_ = app_config::RECONNECT_BACKOFF_MIN_MS;
}
