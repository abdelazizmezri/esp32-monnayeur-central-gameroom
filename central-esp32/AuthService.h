#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "AppState.h"

namespace AuthService {
  void begin(AppState& state);

  bool login(AppState& state, const String& username, const String& password);
  void logout(WebServer& server, AppState& state);

  bool isAuthenticated(WebServer& server, AppState& state);
  bool isAdmin(WebServer& server, AppState& state);
  bool hasValidApiAccess(WebServer& server, AppState& state);
  bool hasAdminAccess(WebServer& server, AppState& state);

  UserAccount* findUser(AppState& state, const String& username);
  UserAccount* currentUser(WebServer& server, AppState& state);

  void requirePageAuth(WebServer& server, AppState& state);
  bool requireApiAuth(WebServer& server, AppState& state);
  bool requireAdminAuth(WebServer& server, AppState& state);

  bool createUser(AppState& state, const String& username, const String& firstName,
                  const String& lastName, const String& password, const String& role,
                  String& error);
  bool updateUser(AppState& state, const String& actorUsername, const String& username,
                  const String& firstName, const String& lastName,
                  const String& password, const String& role, String& error);
  bool deleteUser(AppState& state, const String& actorUsername, const String& username,
                  String& error);
  bool changePassword(AppState& state, const String& username, const String& password,
                      String& error);

  String getMaskedToken(const AppState& state);
  String regenerateApiToken(AppState& state);
}
