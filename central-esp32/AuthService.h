#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "AppState.h"

namespace AuthService {
  void begin(AppState& state);

  bool login(AppState& state, const String& password);
  void logout(AppState& state);

  bool isAuthenticated(WebServer& server, AppState& state);
  bool hasValidApiAccess(WebServer& server, AppState& state);

  void requirePageAuth(WebServer& server, AppState& state);
  bool requireApiAuth(WebServer& server, AppState& state);

  String getMaskedToken(const AppState& state);
  String regenerateApiToken(AppState& state);
}