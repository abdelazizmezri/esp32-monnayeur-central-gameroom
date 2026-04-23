#pragma once

#include "AppState.h"

namespace PostService {
  void begin(AppState& state);

  Post* findById(AppState& state, const String& id);

  bool addPost(AppState& state, const String& id, const String& name, const String& ip, String& error);
  bool updatePost(AppState& state, const String& id, const String& name, const String& ip, String& error);
  bool deletePost(AppState& state, const String& id, String& error);

  bool assignCoins(AppState& state, const String& postId, int coins, String& error);
  bool stopPost(AppState& state, const String& postId, String& error);

  void refreshStatuses(AppState& state);
}