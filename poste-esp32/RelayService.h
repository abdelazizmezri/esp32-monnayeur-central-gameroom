#pragma once

#include "PosteState.h"

namespace RelayService {
  void begin(PosteState& state);
  void update(PosteState& state);

  void setRelay(PosteState& state, bool on);

  void startSession(PosteState& state, unsigned long durationSeconds);
  void addTime(PosteState& state, unsigned long durationSeconds);
  void stopSession(PosteState& state);

  long getRemainingSeconds(const PosteState& state);
}