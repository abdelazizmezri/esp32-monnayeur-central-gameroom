#pragma once

#include "AppState.h"
#include "RevenueJournal.h"

namespace StorageService {
  void load(AppState& state);

  bool saveConfig(const AppState& state);
  void savePosts(const AppState& state);
  void saveAuth(const AppState& state);
  void saveUsers(const AppState& state);
  bool saveCredits(const AppState& state);

  int loadRevenueTransaction(RevenueJournal::Entry& entry);
  bool stageRevenueTransaction(const RevenueJournal::Entry& entry);
  bool clearRevenueTransaction();

  bool exportConfigJson(const AppState& state, String& outputJson);
  bool importConfigJson(AppState& state, const String& inputJson, String& error);
}
