#include "PostService.h"
#include "PosteClient.h"
#include "StorageService.h"
#include "LogService.h"
#include "AppConfig.h"

#include <Arduino.h>

namespace PostService {
  void begin(AppState& state) {
    LogService::info(state, "Gestion des postes prête. Les nouveaux postes s'annoncent automatiquement.");
  }

  Post* findById(AppState& state, const String& id) {
    for (auto& post : state.posts) {
      if (post.id == id) {
        return &post;
      }
    }
    return nullptr;
  }

  PendingPost* findPendingByChipId(AppState& state, const String& chipId) {
    for (auto& pending : state.pendingPosts) {
      if (pending.chipId == chipId) {
        return &pending;
      }
    }
    return nullptr;
  }

  static String trimmedCopy(const String& value) {
    String trimmed = value;
    trimmed.trim();
    return trimmed;
  }

  bool upsertDiscoveredPost(AppState& state, const Post& discovered, bool& added, String& error) {
    added = false;
    String cleanId = trimmedCopy(discovered.id);
    String cleanName = trimmedCopy(discovered.name);
    String cleanIp = trimmedCopy(discovered.ip);

    if (cleanId.isEmpty() || cleanName.isEmpty() || cleanIp.isEmpty()) {
      error = "missing fields";
      return false;
    }

    Post* existingById = findById(state, cleanId);
    if (existingById) {
      existingById->ip = cleanIp;
      existingById->status = discovered.status;
      existingById->relay = discovered.relay;
      existingById->remaining = discovered.remaining;
      existingById->lastSeen = millis();
      StorageService::savePosts(state);
      LogService::info(state, "Poste découvert mis à jour: " + cleanId + " (" + cleanIp + ")");
      return true;
    }

    for (auto& post : state.posts) {
      if (post.ip == cleanIp) {
        post.id = cleanId;
        post.name = cleanName;
        post.status = discovered.status;
        post.relay = discovered.relay;
        post.remaining = discovered.remaining;
        post.lastSeen = millis();
        StorageService::savePosts(state);
        LogService::warn(state, "Poste découvert remplace l'ancien ID sur IP " + cleanIp + ": " + cleanId);
        return true;
      }
    }

    Post post;
    post.id = cleanId;
    post.name = cleanName;
    post.ip = cleanIp;
    post.status = discovered.status;
    post.relay = discovered.relay;
    post.remaining = discovered.remaining;
    post.lastSeen = millis();

    state.posts.push_back(post);
    StorageService::savePosts(state);
    added = true;
    LogService::info(state, "Poste découvert ajouté: " + cleanId + " (" + cleanIp + ")");
    return true;
  }

  bool handleAnnouncement(AppState& state, const String& chipId, const String& ip,
                          bool configured, const String& id, const String& name,
                          const String& status, bool relay, long remaining,
                          String& error) {
    String cleanChipId = trimmedCopy(chipId);
    String cleanIp = trimmedCopy(ip);
    String cleanId = trimmedCopy(id);
    String cleanName = trimmedCopy(name);
    String cleanStatus = trimmedCopy(status);
    if (cleanStatus.isEmpty()) {
      cleanStatus = "idle";
    }

    if (cleanChipId.isEmpty() || cleanIp.isEmpty()) {
      error = "missing fields";
      return false;
    }

    if (configured && !cleanId.isEmpty() && !cleanName.isEmpty()) {
      Post discovered;
      discovered.id = cleanId;
      discovered.name = cleanName;
      discovered.ip = cleanIp;
      discovered.status = cleanStatus;
      discovered.relay = relay;
      discovered.remaining = remaining < 0 ? 0 : remaining;

      bool added = false;
      if (!upsertDiscoveredPost(state, discovered, added, error)) {
        return false;
      }

      for (size_t i = 0; i < state.pendingPosts.size(); i++) {
        if (state.pendingPosts[i].chipId == cleanChipId) {
          state.pendingPosts.erase(state.pendingPosts.begin() + i);
          break;
        }
      }
      return true;
    }

    PendingPost* pending = findPendingByChipId(state, cleanChipId);
    if (pending) {
      pending->ip = cleanIp;
      pending->lastSeen = millis();
      return true;
    }

    PendingPost item;
    item.chipId = cleanChipId;
    item.ip = cleanIp;
    item.lastSeen = millis();
    state.pendingPosts.push_back(item);
    LogService::info(state, "Nouveau poste non configuré annoncé: " + cleanChipId + " (" + cleanIp + ")");
    return true;
  }

  bool configurePendingPost(AppState& state, const String& chipId, const String& id,
                            const String& name, String& error) {
    String cleanChipId = trimmedCopy(chipId);
    String cleanId = trimmedCopy(id);
    String cleanName = trimmedCopy(name);

    if (cleanChipId.isEmpty() || cleanId.isEmpty() || cleanName.isEmpty()) {
      error = "missing fields";
      return false;
    }

    if (findById(state, cleanId)) {
      error = "post id already exists";
      return false;
    }

    PendingPost* pending = findPendingByChipId(state, cleanChipId);
    if (!pending) {
      error = "pending post not found";
      return false;
    }

    String ip = pending->ip;
    if (!PosteClient::configurePost(ip, cleanId, cleanName)) {
      error = "poste unreachable";
      LogService::error(state, "Impossible de configurer le poste " + cleanChipId + " (" + ip + ")");
      return false;
    }

    Post post;
    post.id = cleanId;
    post.name = cleanName;
    post.ip = ip;
    post.status = "idle";
    post.relay = false;
    post.remaining = 0;
    post.lastSeen = millis();

    state.posts.push_back(post);
    StorageService::savePosts(state);

    for (size_t i = 0; i < state.pendingPosts.size(); i++) {
      if (state.pendingPosts[i].chipId == cleanChipId) {
        state.pendingPosts.erase(state.pendingPosts.begin() + i);
        break;
      }
    }

    LogService::info(state, "Poste configuré et ajouté: " + cleanId + " (" + ip + ")");
    return true;
  }

  bool updatePost(AppState& state, const String& id, const String& name, String& error) {
    String cleanId = trimmedCopy(id);
    String cleanName = trimmedCopy(name);

    Post* post = findById(state, cleanId);
    if (!post) {
      error = "post not found";
      return false;
    }

    if (cleanName.isEmpty()) {
      error = "missing fields";
      return false;
    }

    post->name = cleanName;

    StorageService::savePosts(state);
    LogService::info(state, "Nom du poste modifié: " + cleanId);
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
    for (size_t i = 0; i < state.pendingPosts.size();) {
      if (millis() - state.pendingPosts[i].lastSeen > AppConfig::PENDING_POST_TIMEOUT_MS) {
        state.pendingPosts.erase(state.pendingPosts.begin() + i);
      } else {
        i++;
      }
    }

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
