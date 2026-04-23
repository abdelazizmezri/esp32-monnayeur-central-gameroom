#pragma once

#include "AppState.h"

namespace LogService {
  void add(AppState& state, const String& level, const String& message);
  void info(AppState& state, const String& message);
  void warn(AppState& state, const String& message);
  void error(AppState& state, const String& message);
  void clear(AppState& state);
}