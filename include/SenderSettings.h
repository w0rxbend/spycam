#pragma once

#include <stdint.h>

namespace sender {

// Everything TcpFrameSender needs to do its job, handed to it at construction.
//
// This is the sender's entire input surface, written down in one place: reading
// the struct tells you what the class depends on without opening the .cpp. It
// also means a sender is not tied to one global configuration, so two of them
// can point at different servers.
struct Settings {
  // Network the board joins. An empty password means an open network.
  const char *wifiSsid;
  const char *wifiPassword;
  // Wi-Fi modem sleep. Saves power, costs latency and throughput stability.
  bool wifiSleepEnabled;

  // Where frames go. Host may be an IPv4 literal or a DNS name.
  const char *host;
  uint16_t port;
  // Stamped into every frame so one server can tell several boards apart.
  uint32_t cameraId;

  uint32_t wifiConnectTimeoutMs;
  uint32_t tcpConnectTimeoutMs;
  // Budget for one whole frame. Exceeding it drops the connection.
  uint32_t sendTimeoutMs;

  // Retry delay after a failure, doubling from min up to max.
  uint32_t reconnectBackoffMinMs;
  uint32_t reconnectBackoffMaxMs;

  uint32_t statusLogIntervalMs;
  // Only used to warn when a send takes longer than the capture period, which
  // means frames are being dropped before they are ever sent.
  uint32_t frameIntervalMs;
};

} // namespace sender
