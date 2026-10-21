#include "PostService.h"
#include "PosteClient.h"
#include "StorageService.h"
#include "LogService.h"
#include "AppConfig.h"
#include "CreditMath.h"

#include <Arduino.h>

namespace PostService {
  static bool persistPosts(AppState& state, String& error) {
    if (StorageService::savePosts(state)) return true;
    state.creditStorageReady = false;
    state.creditStorageError = "Échec de sauvegarde des postes NVS ; redémarrer avant de continuer.";
    error = state.creditStorageError;
    return false;
  }

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

  static PendingPost* findPendingByChipId(AppState& state, const String& chipId) {
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
    if (!state.creditStorageReady) { error = state.creditStorageError; return false; }
    added = false;
    String cleanId = trimmedCopy(discovered.id);
    String cleanName = trimmedCopy(discovered.name);
    String cleanIp = trimmedCopy(discovered.ip);

    if (cleanId.isEmpty() || cleanName.isEmpty() || cleanIp.isEmpty()) {
      error = "missing fields";
      return false;
    }

    Post* existingById = findById(state, cleanId);
    // L'identité matérielle reste stable lorsque l'IP ou l'ID annoncé change.
    for (auto& post : state.posts) {
      if (!discovered.chipId.isEmpty() && post.chipId == discovered.chipId) {
        existingById = &post;
        break;
      }
    }
    if (existingById) {
      existingById->chipId = discovered.chipId;
      existingById->ip = cleanIp;
      existingById->status = discovered.status;
      existingById->relay = discovered.relay;
      existingById->remaining = discovered.remaining;
      existingById->recoveryPending = discovered.recoveryPending;
      existingById->recoveryRemaining = discovered.recoveryRemaining;
      existingById->lastSeen = millis();
      if (!persistPosts(state, error)) return false;
      LogService::info(state, "Poste découvert mis à jour: " + cleanId + " (" + cleanIp + ")");
      return true;
    }

    for (auto& post : state.posts) {
      if (post.ip == cleanIp) {
        post.chipId = discovered.chipId;
        post.id = cleanId;
        post.name = cleanName;
        post.status = discovered.status;
        post.relay = discovered.relay;
        post.remaining = discovered.remaining;
        post.recoveryPending = discovered.recoveryPending;
        post.recoveryRemaining = discovered.recoveryRemaining;
        post.lastSeen = millis();
        if (!persistPosts(state, error)) return false;
        LogService::warn(state, "Poste découvert remplace l'ancien ID sur IP " + cleanIp + ": " + cleanId);
        return true;
      }
    }

    Post post;
    post.chipId = discovered.chipId;
    post.id = cleanId;
    post.name = cleanName;
    post.ip = cleanIp;
    post.status = discovered.status;
    post.relay = discovered.relay;
    post.remaining = discovered.remaining;
    post.recoveryPending = discovered.recoveryPending;
    post.recoveryRemaining = discovered.recoveryRemaining;
    post.lastSeen = millis();

    state.posts.push_back(post);
    if (!persistPosts(state, error)) { state.posts.pop_back(); return false; }
    added = true;
    LogService::info(state, "Poste découvert ajouté: " + cleanId + " (" + cleanIp + ")");
    return true;
  }

  bool handleAnnouncement(AppState& state, const String& chipId, const String& ip,
                          bool configured, const String& id, const String& name,
                          const String& status, bool relay, long remaining,
                          bool recoveryPending, long recoveryRemaining,
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
      discovered.chipId = cleanChipId;
      discovered.id = cleanId;
      discovered.name = cleanName;
      discovered.ip = cleanIp;
      discovered.status = cleanStatus;
      discovered.relay = relay;
      discovered.remaining = remaining < 0 ? 0 : remaining;
      discovered.recoveryPending = recoveryPending && recoveryRemaining > 0;
      discovered.recoveryRemaining = discovered.recoveryPending ? recoveryRemaining : 0;

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

  bool configurePendingPost(AppState& state, const String& chipId,
                            const String& name, int piecesRequired, String& error) {
    if (!state.creditStorageReady) { error = state.creditStorageError; return false; }
    if (piecesRequired <= 0) { error = "Nombre de pièces requis invalide."; return false; }
    String cleanChipId = trimmedCopy(chipId);
    // Le chipId matériel est déjà unique : il sert d'identifiant technique sans
    // demander à l'administrateur d'en inventer ou d'en saisir un.
    String cleanId = cleanChipId;
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
    post.chipId = cleanChipId;
    post.id = cleanId;
    post.name = cleanName;
    post.ip = ip;
    post.piecesRequired = piecesRequired;
    post.status = "idle";
    post.relay = false;
    post.remaining = 0;
    post.lastSeen = millis();

    state.posts.push_back(post);
    if (!persistPosts(state, error)) { state.posts.pop_back(); return false; }

    for (size_t i = 0; i < state.pendingPosts.size(); i++) {
      if (state.pendingPosts[i].chipId == cleanChipId) {
        state.pendingPosts.erase(state.pendingPosts.begin() + i);
        break;
      }
    }

    LogService::info(state, "Poste configuré et ajouté: " + cleanId + " (" + ip + ")");
    return true;
  }

  bool updatePost(AppState& state, const String& id, const String& name, int piecesRequired, String& error) {
    if (!state.creditStorageReady) { error = state.creditStorageError; return false; }
    if (piecesRequired <= 0) { error = "Nombre de pièces requis invalide."; return false; }
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

    bool rename = cleanName != post->name;
    if (rename && (post->status == "active" || post->remaining > 0 || post->recoveryPending)) {
      error = "post active";
      return false;
    }

    if (rename && !PosteClient::configurePost(post->ip, cleanId, cleanName)) {
      error = "poste unreachable";
      LogService::error(state, "Impossible de modifier l'identité du poste: " + cleanId);
      return false;
    }

    String oldName = post->name;
    int oldPrice = post->piecesRequired;
    post->name = cleanName;
    post->piecesRequired = piecesRequired;
    if (!persistPosts(state, error)) {
      post->name = oldName; post->piecesRequired = oldPrice;
      return false;
    }
    LogService::info(state, "Poste modifié: " + cleanId + ", pièces requises=" + String(piecesRequired));
    return true;
  }

  bool deletePost(AppState& state, const String& id, String& error) {
    if (!state.creditStorageReady) { error = state.creditStorageError; return false; }
    String cleanId = trimmedCopy(id);

    for (size_t i = 0; i < state.posts.size(); i++) {
      if (state.posts[i].id == cleanId) {
        if (state.posts[i].status == "active" || state.posts[i].remaining > 0 ||
            state.posts[i].recoveryPending) {
          error = "post active";
          return false;
        }

        String chipId = state.posts[i].chipId;
        String ip = state.posts[i].ip;
        bool identityCleared = PosteClient::clearIdentity(ip);
        if (!identityCleared) {
          // La suppression locale doit rester possible lorsqu'un poste est
          // définitivement éteint ou n'est plus présent sur le réseau.
          LogService::warn(state, "Poste injoignable pendant la suppression, identité distante non effacée: " + cleanId);
        }

        state.posts.erase(state.posts.begin() + i);
        if (identityCleared && !chipId.isEmpty()) {
          PendingPost* pending = findPendingByChipId(state, chipId);
          if (pending) {
            pending->ip = ip;
            pending->lastSeen = millis();
          } else {
            PendingPost item;
            item.chipId = chipId;
            item.ip = ip;
            item.lastSeen = millis();
            state.pendingPosts.push_back(item);
          }
        }
        if (!persistPosts(state, error)) return false;
        if (identityCleared) {
          LogService::warn(state, "Poste supprimé et remis en découverte: " + cleanId);
        } else {
          LogService::warn(state, "Poste supprimé localement: " + cleanId);
        }
        return true;
      }
    }

    error = "post not found";
    return false;
  }

  bool assignCoins(AppState& state, const String& postId, int coins, String& error) {
    if (!state.creditStorageReady || state.revenueTransactionPending) {
      error = "Stockage ou encaissement en attente ; affectation suspendue."; return false;
    }
    if (coins <= 0) {
      error = "invalid coins";
      return false;
    }

    Post* post = findById(state, postId);
    if (!post) {
      error = "post not found";
      return false;
    }

    if (post->recoveryPending) {
      error = "recovery pending";
      return false;
    }
    if (post->status != "idle" && post->status != "active") {
      error = "Poste indisponible."; return false;
    }
    int cost;
    long duration;
    if (!CreditMath::assignment(post->piecesRequired, coins, state.coinDurationSeconds,
                                post->status == "active" ? post->remaining : 0, cost, duration)) {
      error = "Coût ou durée hors limite."; return false;
    }
    if (state.availablePieces < cost) { error = "Solde de pièces insuffisant."; return false; }
    if (!StorageService::stageAssignment(postId, state.availablePieces,
                                         state.availablePieces - cost, state.lastRevenueSequence)) {
      state.creditStorageReady = false;
      state.creditStorageError = error = "Préparation du débit NVS impossible ; redémarrer.";
      return false;
    }
    String action = post->status == "active" ? "ADD_TIME" : "START_SESSION";

    if (!PosteClient::sendCommand(post->ip, action, duration)) {
      if (!StorageService::clearAssignment()) {
        state.creditStorageReady = false;
        state.assignmentRecoveryPending = true;
        state.creditStorageError = "Échec de finalisation NVS après erreur réseau ; redémarrer.";
      }
      error = "poste unreachable";
      LogService::error(state, "Échec d'envoi commande vers " + postId);
      return false;
    }

    state.availablePieces -= cost;
    post->remaining = action == "ADD_TIME" ? post->remaining + duration : duration;
    post->status = "active";
    post->relay = true;
    if (!StorageService::saveCredits(state)) {
      state.creditStorageReady = false;
      state.assignmentRecoveryPending = true;
      error = "Crédits affectés au poste, mais sauvegarde NVS échouée ; vérification opérateur nécessaire.";
      state.creditStorageError = error;
      LogService::error(state,error);
      return false;
    }
    if (!StorageService::clearAssignment()) {
      state.creditStorageReady = false;
      state.assignmentRecoveryPending = true;
      state.creditStorageError = error = "Débit enregistré, finalisation NVS en attente ; redémarrer.";
      return false;
    }
    LogService::info(state, "Affectation à " + postId + ": " + String(cost) + " pièce(s), +" + String(duration) + " s");
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
    post->recoveryPending = false;
    post->recoveryRemaining = 0;
    LogService::warn(state, "Poste arrêté manuellement: " + postId);
    return true;
  }

  bool resumeInterruptedSession(AppState& state, const String& postId, String& error) {
    Post* post = findById(state, postId);
    if (!post) {
      error = "post not found";
      return false;
    }

    if (!post->recoveryPending || post->recoveryRemaining <= 0) {
      error = "no recovery pending";
      return false;
    }

    long duration = post->recoveryRemaining;
    if (!PosteClient::sendCommand(post->ip, "START_SESSION", duration)) {
      error = "poste unreachable";
      LogService::error(state, "Impossible de reprendre la session de " + postId);
      return false;
    }

    post->status = "active";
    post->relay = true;
    post->remaining = duration;
    post->recoveryPending = false;
    post->recoveryRemaining = 0;
    LogService::info(state, "Session reprise sur " + postId + ": " +
                     String(duration) + " s");
    return true;
  }

  bool cancelInterruptedSession(AppState& state, const String& postId, String& error) {
    Post* post = findById(state, postId);
    if (!post) {
      error = "post not found";
      return false;
    }

    if (!post->recoveryPending) {
      error = "no recovery pending";
      return false;
    }

    if (!PosteClient::sendCommand(post->ip, "STOP_SESSION")) {
      error = "poste unreachable";
      LogService::error(state, "Impossible d'annuler la reprise de " + postId);
      return false;
    }

    post->status = "idle";
    post->relay = false;
    post->remaining = 0;
    post->recoveryPending = false;
    post->recoveryRemaining = 0;
    LogService::warn(state, "Reprise annulée pour " + postId);
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
