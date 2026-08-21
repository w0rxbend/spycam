#pragma once

#include <stddef.h>
#include <stdint.h>

namespace frame_protocol {

// A destination for framed bytes.
//
// This exists so the framing logic (what bytes go out, and in what order) can be
// written and tested without a TCP socket, a Wi-Fi stack, or an ESP32. The
// firmware's implementation writes to a WiFiClient; the unit tests use an
// in-memory implementation that records everything it receives.
class ByteSink {
public:
  virtual ~ByteSink() = default;

  // Writes all len bytes, or returns false. A partial write is a failure:
  // the stream is left misaligned and the caller must drop the connection.
  virtual bool write(const uint8_t *data, size_t len) = 0;
};

} // namespace frame_protocol
