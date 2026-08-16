#pragma once

#include <Arduino.h>
#include <vector>
#include "AppConfig.h"
#include "Post.h"

struct EventLogEntry {
  unsigned long ts = 0;
  String level;
  String message;
};

struct PendingPost {
  String chipId;
  String ip;
  unsigned long lastSeen = 0;
};

struct UserAccount {
  String username;
  String firstName;
  String lastName;
  String role = "user";
  String passwordSalt;
  String passwordHash;
};

struct UserSession {
  String token;
  String username;
};

struct AppState {
  std::vector<Post> posts;
  std::vector<PendingPost> pendingPosts;
  std::vector<EventLogEntry> logs;
  std::vector<UserAccount> users;
  std::vector<UserSession> sessions;

  int pulsesPerCoin = AppConfig::DEFAULT_PULSES_PER_COIN;
  int availableCoins = AppConfig::DEFAULT_AVAILABLE_COINS;
  int coinDurationSeconds = AppConfig::DEFAULT_COIN_DURATION_SECONDS;

  volatile unsigned long lastInterruptTime = 0;
  volatile int pulseCount = 0;

  unsigned long lastPostsRefresh = 0;

  // Utilisé uniquement pour migrer l'ancien mot de passe global vers le compte "admin".
  String legacyAdminPassword = AppConfig::DEFAULT_ADMIN_PASSWORD;
  String apiToken = "";
};
