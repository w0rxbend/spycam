#include "TcpFrameSender.h"

#include "AppConfig.h"
#include "FrameProtocol.h"

namespace {
constexpr size_t kSendChunkSize = 4096;
}

TcpFrameSender::TcpFrameSender(const char *host, uint16_t port)
    : host_(host),
      port_(port),
      sequence_(0),
      backoffMs_(app_config::RECONNECT_BACKOFF_MIN_MS)
{
}

void TcpFrameSender::begin()
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.persistent(false);
  client_.setNoDelay(true);
  client_.setTimeout(app_config::SEND_TIMEOUT_MS / 1000);
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

  if (!ensureConnected()) {
    return false;
  }

  uint8_t header[frame_protocol::HEADER_SIZE];
  frame_protocol::buildHeader(header, sequence_++, static_cast<uint32_t>(frame->len), millis());

  const uint32_t startedAt = millis();
  const bool sent = sendAll(header, sizeof(header)) && sendAll(frame->buf, frame->len);
  const uint32_t elapsed = millis() - startedAt;

  if (!sent) {
    Serial.println("TCP send failed; connection will be reopened");
    disconnect();
    return false;
  }

  resetBackoff();
  Serial.printf("Sent frame seq=%lu bytes=%u elapsed=%lums\n",
                static_cast<unsigned long>(sequence_ - 1),
                static_cast<unsigned>(frame->len),
                static_cast<unsigned long>(elapsed));

  if (elapsed > app_config::FRAME_INTERVAL_MS) {
    Serial.printf("Sender is slower than capture interval (%lums > %lums); stale frames will be dropped\n",
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
  Serial.printf("Connecting WiFi SSID=%s\n", app_config::WIFI_SSID);
  WiFi.disconnect(false);
  WiFi.begin(app_config::WIFI_SSID, app_config::WIFI_PASSWORD);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < app_config::WIFI_CONNECT_TIMEOUT_MS) {
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connect timed out");
    waitBackoff();
    return false;
  }

  Serial.print("WiFi connected, IP=");
  Serial.print(WiFi.localIP());
  Serial.print(", RSSI=");
  Serial.println(WiFi.RSSI());
  resetBackoff();
  return true;
}

bool TcpFrameSender::ensureTcpConnected()
{
  if (client_.connected()) {
    return true;
  }

  client_.stop();
  Serial.printf("Connecting TCP %s:%u\n", host_, port_);
  const bool connected = client_.connect(host_, port_, app_config::TCP_CONNECT_TIMEOUT_MS);
  if (!connected) {
    Serial.println("TCP connect failed");
    waitBackoff();
    return false;
  }

  Serial.println("TCP connected");
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
      Serial.println("TCP send timed out");
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
  Serial.printf("Reconnect backoff %lums\n", static_cast<unsigned long>(delayMs));
  vTaskDelay(pdMS_TO_TICKS(delayMs));
}

void TcpFrameSender::resetBackoff()
{
  backoffMs_ = app_config::RECONNECT_BACKOFF_MIN_MS;
}
