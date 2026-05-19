ESP32-CAM TCP JPEG Client

Captures JPEG frames directly from the ESP32-CAM and sends the latest frame to a TCP server.

## Client architecture

```text
camera task -> latest_frame slot -> tcp sender task
```

The camera task captures JPEG frames directly from the ESP32 camera driver. The latest-frame slot stores only one unsent frame at a time. If the TCP sender is slow or the server is unavailable, stale frames are returned to the camera driver and dropped.

The sender task owns all network I/O. It reconnects WiFi and TCP forever with exponential backoff, then resumes streaming from the newest available frame.

## Client-server protocol

Transport is a long-lived raw TCP connection. There is no HTTP, websocket, JSON, delimiter, or text framing.

The client sends a repeated binary frame:

```text
bytes 0..3     magic: 0x4A504753, ASCII "JPGS", uint32 big-endian
bytes 4..7     sequence number, uint32 big-endian
bytes 8..11    JPEG payload length in bytes, uint32 big-endian
bytes 12..15   camera millis() timestamp, uint32 big-endian
bytes 16..N    JPEG payload, exactly payload length bytes
```

After one frame is sent, the next frame starts immediately with another 16-byte header on the same TCP stream.

Receiver rules:

- Read exactly 16 bytes for the header.
- Decode all header fields as big-endian `uint32`.
- Reject the frame if magic is not `JPGS`.
- Read exactly `payload length` bytes for the JPEG.
- Treat TCP disconnects as normal; the ESP32 client will reconnect and continue with later frames.

The sequence number is useful for detecting dropped frames. The timestamp is the ESP32 `millis()` value when the frame header is built; it is not wall-clock time.

## Configuration

Configure WiFi, server address, frame size, JPEG quality, and target FPS in `include/AppConfig.h`.
