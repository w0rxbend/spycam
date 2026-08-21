#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "esp_camera.h"

#include "ByteSink.h"

class TcpFrameSender {
public:
  TcpFrameSender(const char *host, uint16_t port);

  void begin();
  bool ensureConnected();
  bool sendFrame(camera_fb_t *frame);
  void disconnect();

private:
  // Adapts this sender's socket to the transport-agnostic framing code in
  // FrameWriter.h. Framing decides what bytes go out; this decides where.
  class ClientSink final : public frame_protocol::ByteSink {
  public:
    explicit ClientSink(TcpFrameSender &owner) : owner_(owner) {}
    bool write(const uint8_t *data, size_t len) override { return owner_.sendAll(data, len); }

  private:
    TcpFrameSender &owner_;
  };

  bool ensureWifiConnected();
  bool ensureTcpConnected();
  bool sendAll(const uint8_t *data, size_t len);
  void waitBackoff();
  void resetBackoff();

  const char *host_;
  uint16_t port_;
  WiFiClient client_;
  uint32_t sequence_;
  uint32_t backoffMs_;
  uint32_t sentFrames_;
  uint32_t failedSends_;
  uint32_t lastStatusAt_;
};
