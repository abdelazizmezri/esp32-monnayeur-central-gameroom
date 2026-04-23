#pragma once

#include "AppState.h"

namespace StorageService {
  void begin();

  void load(AppState& state);

  void saveConfig(const AppState& state);
  void savePosts(const AppState& state);
  void saveAuth(const AppState& state);
  void saveCredits(const AppState& state);

  bool exportConfigJson(const AppState& state, String& outputJson);
  bool importConfigJson(AppState& state, const String& inputJson, String& error);
}