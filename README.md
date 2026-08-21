ESP32-CAM TCP JPEG Client

Captures JPEG frames directly from the ESP32-CAM and sends the latest frame to a TCP server.

## Client architecture

```text
camera task -> latest_frame slot -> tcp sender task
```

The camera task captures JPEG frames directly from the ESP32 camera driver. The latest-frame slot stores only one unsent frame at a time. If the TCP sender is slow or the server is unavailable, stale frames are returned to the camera driver and dropped.

The sender task owns all network I/O. It reconnects WiFi and TCP forever with exponential backoff, then resumes streaming from the newest available frame.

### Source layout

| File | Role |
| --- | --- |
| `src/main.cpp` | Composition root. The only file that reads `AppConfig.h`: it builds the settings structs, creates the two objects, and starts the two tasks. |
| `include/AppConfig.h` | Every tunable value, plus a throttling guide for when the board cannot keep up. |
| `CameraManager` | Owns the ESP32 camera driver. Takes a `camera::Settings`. |
| `TcpFrameSender` | Owns Wi-Fi, the socket, reconnection, and backoff. Takes a `sender::Settings`. |
| `LatestFrameSlot` | One-slot handoff between the two tasks. Holds at most one unsent frame. |
| `FrameWriter.h` / `ByteSink.h` | The wire format, and the interface it writes to. No Arduino, no sockets, so the host tests exercise it directly. |

`CameraManager` and `TcpFrameSender` receive their configuration at construction rather than reading globals, so each class states its own inputs and neither depends on `AppConfig.h`. Framing is separated from transport by `ByteSink`, which is what lets `test/test_frame_writer` verify the exact byte order a receiver sees without any hardware.

## Client-server protocol

Transport is a long-lived raw TCP connection. There is no HTTP, websocket, JSON, delimiter, or text framing.

The client sends a repeated binary frame:

```text
bytes 0..3     magic: 0x4A504744, ASCII "JPGD", uint32 big-endian
bytes 4..7     sequence number, uint32 big-endian
bytes 8..11    JPEG payload length in bytes, uint32 big-endian
bytes 12..15   camera millis() timestamp, uint32 big-endian
bytes 16..19   camera id, uint32 big-endian
bytes 20..N    JPEG payload, exactly payload length bytes
```

The payload length field counts the JPEG bytes only; it does not include the 4-byte camera id. The camera id (`app_config::CAMERA_ID` in `include/AppConfig.h`) is repeated on every frame — it is not a one-time handshake — so a server can tell several ESP32-CAM devices apart on one port.

After one frame is sent, the next frame starts immediately with another 20-byte prefix (16-byte header plus the 4-byte camera id) on the same TCP stream.

Receiver rules:

- Read exactly 16 bytes for the header.
- Decode all header fields as big-endian `uint32`.
- Reject the frame if magic is not `JPGD`.
- Read exactly 4 bytes for the camera id.
- Read exactly `payload length` bytes for the JPEG.
- Treat TCP disconnects as normal; the ESP32 client will reconnect and continue with later frames.

The sequence number is useful for detecting dropped frames. The timestamp is the ESP32 `millis()` value when the frame header is built; it is not wall-clock time.

## Configuration

Wi-Fi credentials live in `include/credentials.h`, which is gitignored so your password never lands in a commit. Create it before building:

```c
// include/credentials.h
#pragma once
#define WIFI_SSID "your-network"
#define WIFI_PASSWORD "your-password"
```

If the file is absent the build still succeeds, but falls back to the placeholders `<SSID>` / `<PASSWORD>` and the board will never associate.

Everything else — server host and port, camera id, frame size, JPEG quality, target FPS, camera rotation — is edited directly in `include/AppConfig.h`. `CAMERA_ID` is the value the server uses to tell multiple cameras apart.

## Build and flash

Requires [PlatformIO Core](https://docs.platformio.org/en/latest/core/) (`pio`). [`just`](https://github.com/casey/just) is optional but wraps the common tasks.

```sh
pio run -e esp32cam                                    # build firmware
pio test -e native                                     # host-side unit tests
pio run -e esp32cam -t upload --upload-port /dev/ttyUSB0
pio device monitor
```

With `just`:

```sh
just build
just test
just check                 # tests, then build
just flash /dev/ttyUSB0
just monitor /dev/ttyUSB0
just deploy /dev/ttyUSB0   # check, flash, monitor
```

The serial port defaults to `PIO_UPLOAD_PORT`, which the justfile reads from a `.env` file. Run `just --list` for the rest.
