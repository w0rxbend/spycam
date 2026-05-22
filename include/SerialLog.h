#pragma once

#include <Arduino.h>

namespace serial_log {

enum class Level : uint8_t {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
};

void begin(uint32_t baud, Level level);
void setLevel(Level level);
bool enabled(Level level);

void error(const char *format, ...);
void warn(const char *format, ...);
void info(const char *format, ...);
void debug(const char *format, ...);

} // namespace serial_log
