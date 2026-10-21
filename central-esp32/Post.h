#pragma once

#include <Arduino.h>
#include "AppConfig.h"

struct Post {
  String chipId;
  String id;
  String name;
  String ip;
  int piecesRequired = AppConfig::DEFAULT_PIECES_REQUIRED;
  String status = "idle";
  bool relay = false;
  long remaining = 0;
  bool recoveryPending = false;
  long recoveryRemaining = 0;
  unsigned long lastSeen = 0;
};
