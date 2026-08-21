#include <unity.h>

#include <string.h>

#include "FrameWriter.h"

namespace {

// Records every byte handed to it, plus the size of each individual write, so a
// test can assert both the resulting stream and how it was chunked.
class RecordingSink final : public frame_protocol::ByteSink {
public:
  bool write(const uint8_t *data, size_t len) override
  {
    ++writeCount;
    lastWriteLen = len;
    if (failNextWrite) {
      return false;
    }
    if (total + len > sizeof(bytes)) {
      return false;
    }
    memcpy(bytes + total, data, len);
    total += len;
    return true;
  }

  uint8_t bytes[64] = {};
  size_t total = 0;
  size_t writeCount = 0;
  size_t lastWriteLen = 0;
  bool failNextWrite = false;
};

uint32_t readU32Be(const uint8_t *src)
{
  return (static_cast<uint32_t>(src[0]) << 24) |
         (static_cast<uint32_t>(src[1]) << 16) |
         (static_cast<uint32_t>(src[2]) << 8) |
         static_cast<uint32_t>(src[3]);
}

const uint8_t kPayload[3] = {0xAA, 0xBB, 0xCC};

frame_protocol::FrameMetadata metadata()
{
  frame_protocol::FrameMetadata meta;
  meta.sequence = 7;
  meta.payloadLen = sizeof(kPayload);
  meta.timestampMs = 1234;
  meta.cameraId = 3;
  return meta;
}

// This is the test the old suite could not express: it pins the order of the
// fields as they actually reach the socket, not just how one header encodes.
void test_write_frame_emits_header_then_camera_id_then_payload()
{
  RecordingSink sink;

  TEST_ASSERT_TRUE(frame_protocol::writeFrame(sink, metadata(), kPayload));

  TEST_ASSERT_EQUAL_UINT(frame_protocol::PREFIX_SIZE + sizeof(kPayload), sink.total);
  TEST_ASSERT_EQUAL_HEX32(0x4A504744, readU32Be(sink.bytes + 0));  // magic, ASCII "JPGD"
  TEST_ASSERT_EQUAL_UINT32(7, readU32Be(sink.bytes + 4));          // sequence
  TEST_ASSERT_EQUAL_UINT32(3, readU32Be(sink.bytes + 8));          // JPEG length
  TEST_ASSERT_EQUAL_UINT32(1234, readU32Be(sink.bytes + 12));      // timestamp
  TEST_ASSERT_EQUAL_UINT32(3, readU32Be(sink.bytes + 16));         // camera id
  TEST_ASSERT_EQUAL_HEX8(0xAA, sink.bytes[20]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, sink.bytes[21]);
  TEST_ASSERT_EQUAL_HEX8(0xCC, sink.bytes[22]);
}

// The declared length must cover the JPEG only. If the camera id were ever
// folded into it, a receiver would read 4 bytes of id as image data and then
// look for the next header 4 bytes early, desyncing the stream permanently.
void test_declared_length_excludes_the_camera_id()
{
  RecordingSink sink;

  TEST_ASSERT_TRUE(frame_protocol::writeFrame(sink, metadata(), kPayload));

  const uint32_t declaredLen = readU32Be(sink.bytes + 8);
  TEST_ASSERT_EQUAL_UINT32(sizeof(kPayload), declaredLen);
  TEST_ASSERT_EQUAL_UINT(frame_protocol::PREFIX_SIZE + declaredLen, sink.total);
}

void test_prefix_is_written_as_one_contiguous_chunk()
{
  RecordingSink sink;

  TEST_ASSERT_TRUE(frame_protocol::writeFrame(sink, metadata(), kPayload));

  TEST_ASSERT_EQUAL_UINT(2, sink.writeCount);
  TEST_ASSERT_EQUAL_UINT(sizeof(kPayload), sink.lastWriteLen);
}

void test_write_frame_reports_failure_and_stops_when_the_sink_fails()
{
  RecordingSink sink;
  sink.failNextWrite = true;

  TEST_ASSERT_FALSE(frame_protocol::writeFrame(sink, metadata(), kPayload));

  TEST_ASSERT_EQUAL_UINT(1, sink.writeCount);  // payload not attempted
  TEST_ASSERT_EQUAL_UINT(0, sink.total);
}

void test_write_frame_rejects_an_empty_or_missing_payload()
{
  RecordingSink sink;
  frame_protocol::FrameMetadata meta = metadata();
  meta.payloadLen = 0;

  TEST_ASSERT_FALSE(frame_protocol::writeFrame(sink, meta, kPayload));
  TEST_ASSERT_FALSE(frame_protocol::writeFrame(sink, metadata(), nullptr));

  TEST_ASSERT_EQUAL_UINT(0, sink.writeCount);
}

} // namespace

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_write_frame_emits_header_then_camera_id_then_payload);
  RUN_TEST(test_declared_length_excludes_the_camera_id);
  RUN_TEST(test_prefix_is_written_as_one_contiguous_chunk);
  RUN_TEST(test_write_frame_reports_failure_and_stops_when_the_sink_fails);
  RUN_TEST(test_write_frame_rejects_an_empty_or_missing_payload);
  return UNITY_END();
}
