set dotenv-load

firmware_env := env_var_or_default("PIO_FIRMWARE_ENV", "esp32cam")
test_env := env_var_or_default("PIO_TEST_ENV", "native")
upload_port := env_var_or_default("PIO_UPLOAD_PORT", "")
monitor_port := env_var_or_default("PIO_MONITOR_PORT", upload_port)

default:
    @just --list

# Build firmware for the ESP32-CAM target.
build env=firmware_env:
    pio run -e {{env}}

# Run fast host-side unit tests.
test env=test_env:
    pio test -e {{env}}

# Run tests for a specific PlatformIO environment.
test-env env:
    pio test -e {{env}}

# Build firmware and run native unit tests.
check: test build

# Flash firmware to the device. Override port with: just flash /dev/ttyUSB0
flash port=upload_port env=firmware_env:
    #!/usr/bin/env sh
    if [ -n "{{port}}" ]; then
      pio run -e {{env}} -t upload --upload-port "{{port}}"
    else
      pio run -e {{env}} -t upload
    fi

# Open serial monitor. Override port with: just monitor /dev/ttyUSB0
monitor port=monitor_port env=firmware_env:
    #!/usr/bin/env sh
    if [ -n "{{port}}" ]; then
      pio device monitor -e {{env}} --port "{{port}}"
    else
      pio device monitor -e {{env}}
    fi

# Flash firmware and then open the serial monitor.
flash-monitor port=upload_port env=firmware_env:
    just flash "{{port}}" {{env}}
    just monitor "{{port}}" {{env}}

# List connected serial devices.
ports:
    pio device list

# Clean build artifacts for one environment.
clean env=firmware_env:
    pio run -e {{env}} -t clean

# Clean all PlatformIO build artifacts.
clean-all:
    pio run -t clean

# Show resolved PlatformIO project configuration.
config:
    pio project config

# Reinstall/update PlatformIO dependencies.
deps:
    pio pkg install

# Show firmware binary size.
size env=firmware_env:
    pio run -e {{env}} -t size

# Build, flash, and monitor in one command.
deploy port=upload_port env=firmware_env:
    just check
    just flash-monitor "{{port}}" {{env}}
