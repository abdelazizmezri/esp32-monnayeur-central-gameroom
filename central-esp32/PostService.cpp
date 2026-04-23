#include "PostService.h"
#include "PosteClient.h"
#include "StorageService.h"
#include "LogService.h"
#include "AppConfig.h"

#include <Arduino.h>

namespace PostService {
  void begin(AppState& state) {
    if (state.posts.empty()) {
      state.posts.push_back({"post1", "Poste 1", "192.168.1.101", "idle", false, 0, 0});
      StorageService::savePosts(state);
      LogService::info(state, "Poste par défaut créé.");
    }
  }

  Post* findById(AppState& state, const String& id) {
    for (auto& post : state.posts) {
      if (post.id == id) {
        return &post;
      }
    }
    return nullptr;
  }

  static String trimmedCopy(const String& value) {
    String trimmed = value;
    trimmed.trim();
    return trimmed;
  }

  static bool ipExistsExcluding(AppState& state, const String& ip, const String& excludedId) {
    for (auto& post : state.posts) {
      if (post.id != excludedId && post.ip == ip) {
        return true;
      }
    }
    return false;
  }

  bool addPost(AppState& state, const String& id, const String& name, const String& ip, String& error) {
    String cleanId = trimmedCopy(id);
    String cleanName = trimmedCopy(name);
    String cleanIp = trimmedCopy(ip);

    if (cleanId.isEmpty() || cleanName.isEmpty() || cleanIp.isEmpty()) {
      error = "missing fields";
      return false;
    }

    if (findById(state, cleanId)) {
      error = "post id already exists";
      return false;
    }

    if (ipExistsExcluding(state, cleanIp, "")) {
      error = "post ip already exists";
      return false;
    }

    Post post;
    post.id = cleanId;
    post.name = cleanName;
    post.ip = cleanIp;
    post.status = "idle";
    post.relay = false;
    post.remaining = 0;
    post.lastSeen = 0;

    state.posts.push_back(post);
    StorageService::savePosts(state);
    LogService::info(state, "Poste ajouté: " + cleanId + " (" + cleanIp + ")");
    return true;
  }

  bool updatePost(AppState& state, const String& id, const String& name, const String& ip, String& error) {
    String cleanId = trimmedCopy(id);
    String cleanName = trimmedCopy(name);
    String cleanIp = trimmedCopy(ip);

    Post* post = findById(state, cleanId);
    if (!post) {
      error = "post not found";
      return false;
    }

    if (cleanName.isEmpty() || cleanIp.isEmpty()) {
      error = "missing fields";
      return false;
    }

    if (ipExistsExcluding(state, cleanIp, cleanId)) {
      error = "post ip already exists";
      return false;
    }

    post->name = cleanName;
    post->ip = cleanIp;

    StorageService::savePosts(state);
    LogService::info(state, "Poste modifié: " + cleanId);
    return true;
  }

  bool deletePost(AppState& state, const String& id, String& error) {
    for (size_t i = 0; i < state.posts.size(); i++) {
      if (state.posts[i].id == id) {
        state.posts.erase(state.posts.begin() + i);
        StorageService::savePosts(state);
        LogService::warn(state, "Poste supprimé: " + id);
        return true;
      }
    }

    error = "post not found";
    return false;
  }

  bool assignCoins(AppState& state, const String& postId, int coins, String& error) {
    if (coins <= 0) {
      error = "invalid coins";
      return false;
    }

    if (state.availableCoins < coins) {
      error = "not enough coins";
      return false;
    }

    Post* post = findById(state, postId);
    if (!post) {
      error = "post not found";
      return false;
    }

    long duration = (long)coins * state.coinDurationSeconds;
    String action = post->status == "active" ? "ADD_TIME" : "START_SESSION";

    if (!PosteClient::sendCommand(post->ip, action, duration)) {
      error = "poste unreachable";
      LogService::error(state, "Échec d'envoi commande vers " + postId);
      return false;
    }

    state.availableCoins -= coins;
    StorageService::saveCredits(state);
    LogService::info(state, "Crédits affectés à " + postId + ": " + String(coins) + " coin(s)");
    return true;
  }

  bool stopPost(AppState& state, const String& postId, String& error) {
    Post* post = findById(state, postId);
    if (!post) {
      error = "post not found";
      return false;
    }

    if (!PosteClient::sendCommand(post->ip, "STOP_SESSION")) {
      error = "poste unreachable";
      LogService::error(state, "Impossible d'arrêter " + postId);
      return false;
    }

    post->status = "idle";
    post->relay = false;
    post->remaining = 0;
    LogService::warn(state, "Poste arrêté manuellement: " + postId);
    return true;
  }

  void refreshStatuses(AppState& state) {
    if (state.posts.empty()) {
      return;
    }

    if (millis() - state.lastPostsRefresh < AppConfig::POST_REFRESH_INTERVAL_MS) {
      return;
    }

    static size_t nextPostIndex = 0;
    if (nextPostIndex >= state.posts.size()) {
      nextPostIndex = 0;
    }

    Post& post = state.posts[nextPostIndex++];
    bool ok = PosteClient::fetchStatus(post);
    if (!ok && millis() - post.lastSeen > AppConfig::POST_OFFLINE_TIMEOUT_MS) {
      post.status = "offline";
      post.relay = false;
      post.remaining = 0;
    }

    state.lastPostsRefresh = millis();
  }
}
