#include "LogService.h"
#include <Arduino.h>

namespace LogService {
  static const size_t MAX_LOGS = 100;

  void add(AppState& state, const String& level, const String& message) {
    EventLogEntry entry;
    entry.ts = millis();
    entry.level = level;
    entry.message = message;

    state.logs.push_back(entry);
    if (state.logs.size() > MAX_LOGS) {
      state.logs.erase(state.logs.begin());
    }

    Serial.print("[");
    Serial.print(level);
    Serial.print("] ");
    Serial.println(message);
  }

  void info(AppState& state, const String& message) {
    add(state, "INFO", message);
  }

  void warn(AppState& state, const String& message) {
    add(state, "WARN", message);
  }

  void error(AppState& state, const String& message) {
    add(state, "ERROR", message);
  }

  void clear(AppState& state) {
    state.logs.clear();
  }
}