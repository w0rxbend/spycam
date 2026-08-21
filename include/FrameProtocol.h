#pragma once

#include <stddef.h>
#include <stdint.h>

namespace frame_protocol {

constexpr uint32_t MAGIC_JPGD = 0x4A504744;
constexpr size_t HEADER_SIZE = 16;
constexpr size_t CAMERA_ID_SIZE = 4;
// Every frame is preceded by the header immediately followed by the camera id.
// The header's length field counts the JPEG bytes only, so a receiver reads
// PREFIX_SIZE bytes and then exactly payloadLen more.
constexpr size_t PREFIX_SIZE = HEADER_SIZE + CAMERA_ID_SIZE;

inline void writeU32Be(uint8_t *dst, uint32_t value)
{
  dst[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
  dst[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
  dst[3] = static_cast<uint8_t>(value & 0xFF);
}

inline void buildHeader(uint8_t *header, uint32_t sequence, uint32_t payloadLen, uint32_t timestampMs)
{
  writeU32Be(header + 0, MAGIC_JPGD);
  writeU32Be(header + 4, sequence);
  writeU32Be(header + 8, payloadLen);
  writeU32Be(header + 12, timestampMs);
}

inline void buildCameraId(uint8_t *dst, uint32_t cameraId)
{
  writeU32Be(dst, cameraId);
}

} // namespace frame_protocol
