#pragma once

#include <Arduino.h>

struct Post {
  String chipId;
  String id;
  String name;
  String ip;
  String status = "idle";
  bool relay = false;
  long remaining = 0;
  unsigned long lastSeen = 0;
};
