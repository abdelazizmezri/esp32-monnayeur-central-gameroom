#pragma once

#include "AppState.h"

namespace CoinService {
  // Suspendre la capture pendant la vidange et l'engagement d'une configuration.
  struct ConfigurationPause {
    ConfigurationPause();
    ~ConfigurationPause();
    ConfigurationPause(const ConfigurationPause&) = delete;
    ConfigurationPause& operator=(const ConfigurationPause&) = delete;
  };
  void begin(AppState& state);
  void update(AppState& state);
  void publishConfiguration(const AppState& state);
  bool settle(AppState& state);
}
