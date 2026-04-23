#pragma once

#include <Arduino.h>

struct PosteState {
  String chipId;
  bool configured = false;

  String id;
  String name;
  String status;

  bool relayState = false;
  unsigned long endTimeMs = 0;

  bool wifiConnected = false;
  bool wifiConfigPortalActive = false;
  String wifiSsid = "";
  String wifiIp = "";
};
