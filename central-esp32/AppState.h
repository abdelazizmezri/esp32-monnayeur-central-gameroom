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

struct AppState {
  std::vector<Post> posts;
  std::vector<EventLogEntry> logs;

  int pulsesPerCoin = AppConfig::DEFAULT_PULSES_PER_COIN;
  int availableCoins = AppConfig::DEFAULT_AVAILABLE_COINS;
  int coinDurationSeconds = AppConfig::DEFAULT_COIN_DURATION_SECONDS;

  volatile unsigned long lastInterruptTime = 0;
  volatile int pulseCount = 0;

  unsigned long lastPostsRefresh = 0;

  String adminPassword = AppConfig::DEFAULT_ADMIN_PASSWORD;
  String apiToken = "";
  String sessionToken = "";
};
