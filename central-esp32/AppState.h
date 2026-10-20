#pragma once

#include <Arduino.h>
#include <vector>
#include "AppConfig.h"
#include "Post.h"
#include "Money.h"

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
  uint8_t coinCurrency = Money::TND;
  uint32_t pulseValueMinor = AppConfig::DEFAULT_PULSE_VALUE_MILLIMES;
  uint32_t dzdToTndRateMicros = 0; // Non renseigné ; interdit en mode DZD.
  int availableCoins = AppConfig::DEFAULT_AVAILABLE_COINS;
  int coinDurationSeconds = AppConfig::DEFAULT_COIN_DURATION_SECONDS;

  volatile int pulseCount = 0;
  int pendingPulses = 0;
  int pendingPulsesPerCoin = AppConfig::DEFAULT_PULSES_PER_COIN;
  uint64_t lastRevenueSequence = 0;
  bool creditStorageReady = true;
  bool revenueTransactionPending = false;

  bool revenueStorageReady = false;
  bool revenueTimeReady = false;
  String revenueStorageError;

  unsigned long lastPostsRefresh = 0;

  // Utilisé uniquement pour migrer l'ancien mot de passe global vers le compte "admin".
  String legacyAdminPassword = AppConfig::DEFAULT_ADMIN_PASSWORD;
  String apiToken = "";
};
