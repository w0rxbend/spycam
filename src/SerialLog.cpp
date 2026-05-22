#include "SerialLog.h"

#include <cstdarg>
#include <cstdio>

namespace {

serial_log::Level currentLevel = serial_log::Level::Info;

const char *levelName(serial_log::Level level)
{
  switch (level) {
    case serial_log::Level::Error:
      return "ERROR";
    case serial_log::Level::Warn:
      return "WARN";
    case serial_log::Level::Info:
      return "INFO";
    case serial_log::Level::Debug:
      return "DEBUG";
  }
  return "?";
}

void logMessage(serial_log::Level level, const char *format, va_list args)
{
  if (!serial_log::enabled(level)) {
    return;
  }

  char message[192];
  vsnprintf(message, sizeof(message), format, args);
  Serial.printf("[%10lu] %-5s %s\n",
                static_cast<unsigned long>(millis()),
                levelName(level),
                message);
}

} // namespace

namespace serial_log {

void begin(uint32_t baud, Level level)
{
  currentLevel = level;
  Serial.begin(baud);
  delay(200);
  Serial.println("--- Serial log started ---");
}

void setLevel(Level level)
{
  currentLevel = level;
}

bool enabled(Level level)
{
  return static_cast<uint8_t>(level) <= static_cast<uint8_t>(currentLevel);
}

void error(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Error, format, args);
  va_end(args);
}

void warn(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Warn, format, args);
  va_end(args);
}

void info(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Info, format, args);
  va_end(args);
}

void debug(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  logMessage(Level::Debug, format, args);
  va_end(args);
}

} // namespace serial_log
