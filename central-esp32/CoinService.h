#pragma once

#include "AppState.h"

namespace CoinService {
  void begin(AppState& state);
  void update(AppState& state);
  void onInterrupt();
}