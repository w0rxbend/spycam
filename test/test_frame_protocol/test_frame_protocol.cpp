#include <unity.h>

#include "FrameProtocol.h"

namespace {

uint32_t readU32Be(const uint8_t *src)
{
  return (static_cast<uint32_t>(src[0]) << 24) |
         (static_cast<uint32_t>(src[1]) << 16) |
         (static_cast<uint32_t>(src[2]) << 8) |
         static_cast<uint32_t>(src[3]);
}

void test_write_u32_be_encodes_most_significant_byte_first()
{
  uint8_t bytes[4] = {};

  frame_protocol::writeU32Be(bytes, 0x12345678);

  TEST_ASSERT_EQUAL_HEX8(0x12, bytes[0]);
  TEST_ASSERT_EQUAL_HEX8(0x34, bytes[1]);
  TEST_ASSERT_EQUAL_HEX8(0x56, bytes[2]);
  TEST_ASSERT_EQUAL_HEX8(0x78, bytes[3]);
}

void test_build_header_encodes_protocol_fields()
{
  uint8_t header[frame_protocol::HEADER_SIZE] = {};

  frame_protocol::buildHeader(header, 42, 123456, 987654321);

  TEST_ASSERT_EQUAL_HEX32(0x4A504744, readU32Be(header + 0)); // ASCII "JPGD"
  TEST_ASSERT_EQUAL_UINT32(42, readU32Be(header + 4));
  TEST_ASSERT_EQUAL_UINT32(123456, readU32Be(header + 8));
  TEST_ASSERT_EQUAL_UINT32(987654321, readU32Be(header + 12));
}

void test_build_header_is_exactly_16_bytes()
{
  TEST_ASSERT_EQUAL_UINT(16, frame_protocol::HEADER_SIZE);
}

void test_build_camera_id_encodes_big_endian_id()
{
  uint8_t bytes[frame_protocol::CAMERA_ID_SIZE] = {};

  frame_protocol::buildCameraId(bytes, 17);

  TEST_ASSERT_EQUAL_UINT32(17, readU32Be(bytes));
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_write_u32_be_encodes_most_significant_byte_first);
  RUN_TEST(test_build_header_encodes_protocol_fields);
  RUN_TEST(test_build_header_is_exactly_16_bytes);
  RUN_TEST(test_build_camera_id_encodes_big_endian_id);
  return UNITY_END();
}
