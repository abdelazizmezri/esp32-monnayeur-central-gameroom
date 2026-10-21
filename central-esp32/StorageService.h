#pragma once

#include "AppState.h"
#include "RevenueJournal.h"

namespace StorageService {
  void load(AppState& state);

  bool saveConfig(const AppState& state);
  bool savePosts(const AppState& state);
  bool saveAuth(const AppState& state);
  bool saveUsers(const AppState& state);
  bool saveCredits(const AppState& state);
  bool stageAssignment(const String& postId, int before, int after, uint64_t sequence);
  bool clearAssignment();
  bool resolveAssignment(AppState& state, bool debit, String& error);

  int loadRevenueTransaction(RevenueJournal::Entry& entry);
  bool stageRevenueTransaction(const RevenueJournal::Entry& entry);
  bool clearRevenueTransaction();

  bool exportConfigJson(const AppState& state, String& outputJson);
  bool importConfigJson(AppState& state, const String& inputJson, String& error);
}
