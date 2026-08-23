#pragma once

#include <Arduino.h>

struct PosteState {
  String chipId;
  bool configured = false;

  String id;
  String name;

  bool wifiConnected = false;
  bool wifiConfigPortalActive = false;
  String wifiIp = "";
};
