#pragma once

#include "ByteSink.h"
#include "FrameProtocol.h"

namespace frame_protocol {

// Everything the receiver needs to interpret one JPEG frame.
struct FrameMetadata {
  uint32_t sequence;
  uint32_t payloadLen;
  uint32_t timestampMs;
  uint32_t cameraId;
};

// Writes one complete frame to sink: the 16-byte header, then the 4-byte camera
// id, then exactly payloadLen JPEG bytes.
//
// The header and camera id go out in a single write because they are one
// contiguous 20-byte prefix on the wire. Splitting them across two writes would
// produce identical bytes on a TCP stream, but keeping them together means the
// prefix can never be torn by a partial-write failure between the two.
inline bool writeFrame(ByteSink &sink, const FrameMetadata &meta, const uint8_t *payload)
{
  if (payload == nullptr || meta.payloadLen == 0) {
    return false;
  }

  uint8_t prefix[PREFIX_SIZE];
  buildHeader(prefix, meta.sequence, meta.payloadLen, meta.timestampMs);
  buildCameraId(prefix + HEADER_SIZE, meta.cameraId);

  if (!sink.write(prefix, sizeof(prefix))) {
    return false;
  }
  return sink.write(payload, meta.payloadLen);
}

} // namespace frame_protocol
