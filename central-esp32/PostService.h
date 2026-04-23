#pragma once

#include "AppState.h"

namespace PostService {
  void begin(AppState& state);

  Post* findById(AppState& state, const String& id);
  PendingPost* findPendingByChipId(AppState& state, const String& chipId);

  bool upsertDiscoveredPost(AppState& state, const Post& discovered, bool& added, String& error);
  bool handleAnnouncement(AppState& state, const String& chipId, const String& ip,
                          bool configured, const String& id, const String& name,
                          const String& status, bool relay, long remaining,
                          String& error);
  bool configurePendingPost(AppState& state, const String& chipId, const String& id,
                            const String& name, String& error);
  bool updatePost(AppState& state, const String& id, const String& name, String& error);
  bool deletePost(AppState& state, const String& id, String& error);

  bool assignCoins(AppState& state, const String& postId, int coins, String& error);
  bool stopPost(AppState& state, const String& postId, String& error);

  void refreshStatuses(AppState& state);
}
