#include "WebRoutes.h"
#include "AppConfig.h"
#include "WebPage.h"
#include "WebAssets.h"
#include "PostService.h"
#include "StorageService.h"
#include "AuthService.h"
#include "WifiProvisioning.h"
#include "PosteClient.h"
#include "LogService.h"

#include <ArduinoJson.h>

static WebServer* gServer = nullptr;
static AppState* gState = nullptr;
static const char* AUTH_HEADERS[] = {"Authorization", "Cookie"};

static void sendJsonError(int code, const String& message) {
  DynamicJsonDocument doc(256);
  doc["error"] = message;
  String json;
  serializeJson(doc, json);
  gServer->send(code, "application/json", json);
}

static bool parseJsonBody(DynamicJsonDocument& doc) {
  if (!gServer->hasArg("plain")) {
    sendJsonError(400, "missing body");
    return false;
  }

  DeserializationError err = deserializeJson(doc, gServer->arg("plain"));
  if (err) {
    sendJsonError(400, "invalid json");
    return false;
  }

  return true;
}

static bool hasPosteAuth() {
  if (!gServer->hasHeader("Authorization")) {
    return false;
  }

  String expected = String("Bearer ") + AppConfig::POSTE_COMMAND_TOKEN;
  return gServer->header("Authorization") == expected;
}

static void handleAppPage() {
  if (!AuthService::isAuthenticated(*gServer, *gState)) {
    gServer->sendHeader("Location", "/login", true);
    gServer->send(302, "text/plain", "");
    return;
  }

  gServer->send(200, "text/html", WebPage::dashboardPage());
}

static void handleAdminPage() {
  if (!AuthService::isAuthenticated(*gServer, *gState)) {
    gServer->sendHeader("Location", "/login", true);
    gServer->send(302, "text/plain", "");
    return;
  }

  if (!AuthService::isAdmin(*gServer, *gState)) {
    gServer->sendHeader("Location", "/", true);
    gServer->send(302, "text/plain", "");
    return;
  }

  gServer->send(200, "text/html", WebPage::dashboardPage());
}

static void handleLoginPage() {
  gServer->sendHeader("Cache-Control", "no-store");
  gServer->send(200, "text/html", WebPage::loginPage());
}

static void handleLoginStationAsset() {
  gServer->sendHeader("Cache-Control", "public, max-age=31536000, immutable");
  gServer->send_P(200,
                  "image/jpeg",
                  reinterpret_cast<PGM_P>(WebAssets::LOGIN_STATION_JPG),
                  WebAssets::LOGIN_STATION_JPG_LENGTH);
}

static void handleGameRoomLogoAsset() {
  gServer->sendHeader("Cache-Control", "public, max-age=31536000, immutable");
  gServer->send_P(200,
                  "image/webp",
                  reinterpret_cast<PGM_P>(WebAssets::GAME_ROOM_LOGO_WEBP),
                  WebAssets::GAME_ROOM_LOGO_WEBP_LENGTH);
}

static void handlePlayStationMarkAsset() {
  gServer->sendHeader("Cache-Control", "public, max-age=31536000, immutable");
  gServer->send_P(200,
                  "image/png",
                  reinterpret_cast<PGM_P>(WebAssets::PLAYSTATION_MARK_PNG),
                  WebAssets::PLAYSTATION_MARK_PNG_LENGTH);
}

static void handleLogin() {
  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String username = doc["username"] | "";
  String password = doc["password"] | "";
  bool remember = doc["remember"] | false;
  if (!AuthService::login(*gState, username, password)) {
    sendJsonError(401, "Nom d'utilisateur ou mot de passe incorrect");
    return;
  }

  String sessionToken = gState->sessions.back().token;
  UserAccount* user = AuthService::findUser(*gState, username);
  LogService::info(*gState, "Connexion réussie: " + (user ? user->username : username));
  String cookie = "ESPSESSION=" + sessionToken + "; Path=/; HttpOnly; SameSite=Strict";
  if (remember) {
    cookie += "; Max-Age=2592000";
  }
  gServer->sendHeader("Set-Cookie", cookie);
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleLogout() {
  AuthService::logout(*gServer, *gState);
  gServer->sendHeader("Set-Cookie", "ESPSESSION=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleGetPosts() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("posts");

  for (auto& post : gState->posts) {
    JsonObject p = arr.createNestedObject();
    p["chipId"] = post.chipId;
    p["id"] = post.id;
    p["name"] = post.name;
    p["ip"] = post.ip;
    p["status"] = post.status;
    p["relay"] = post.relay;
    p["remaining"] = post.remaining;
    p["recoveryPending"] = post.recoveryPending;
    p["recoveryRemaining"] = post.recoveryRemaining;
  }

  JsonArray pendingArr = doc.createNestedArray("pendingPosts");
  bool adminAccess = AuthService::hasAdminAccess(*gServer, *gState);
  if (adminAccess) {
    for (auto& pending : gState->pendingPosts) {
      JsonObject p = pendingArr.createNestedObject();
      p["chipId"] = pending.chipId;
      p["ip"] = pending.ip;
      p["lastSeen"] = pending.lastSeen;
    }
  }

  doc["availableCoins"] = gState->availableCoins;
  doc["coinDurationSeconds"] = gState->coinDurationSeconds;
  doc["pulsesPerCoin"] = gState->pulsesPerCoin;
  doc["accessRole"] = adminAccess ? "admin" : "user";

  UserAccount* currentUser = AuthService::currentUser(*gServer, *gState);
  if (currentUser) {
    JsonObject identity = doc.createNestedObject("currentUser");
    identity["username"] = currentUser->username;
    identity["firstName"] = currentUser->firstName;
    identity["lastName"] = currentUser->lastName;
    identity["role"] = currentUser->role;
  }
  if (adminAccess) {
    doc["apiTokenMasked"] = AuthService::getMaskedToken(*gState);
  }

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleUpdatePost() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::updatePost(*gState, doc["id"] | "", doc["name"] | "", error);

  if (!ok) {
    int code = 400;
    if (error == "post not found") code = 404;
    else if (error == "poste unreachable") code = 502;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleDeletePost() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(128);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::deletePost(*gState, doc["id"] | "", error);

  if (!ok) {
    int code = 400;
    if (error == "post not found") code = 404;
    else if (error == "poste unreachable") code = 502;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleAssign() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::assignCoins(*gState, doc["post_id"] | "", doc["coins"] | 0, error);

  if (!ok) {
    int code = 400;
    if (error == "post not found") code = 404;
    else if (error == "poste unreachable") code = 502;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleSimulateCoin() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  gState->availableCoins += 1;
  StorageService::saveCredits(*gState);
  LogService::info(*gState, "Simulation monnayeur: +1 coin, total=" + String(gState->availableCoins));

  DynamicJsonDocument doc(128);
  doc["ok"] = true;
  doc["availableCoins"] = gState->availableCoins;

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleConfig() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  if (doc.containsKey("coinDurationSeconds")) {
    int value = doc["coinDurationSeconds"];
    if (value <= 0) {
      sendJsonError(400, "invalid coinDurationSeconds");
      return;
    }
    gState->coinDurationSeconds = value;
  }

  if (doc.containsKey("pulsesPerCoin")) {
    int value = doc["pulsesPerCoin"];
    if (value <= 0) {
      sendJsonError(400, "invalid pulsesPerCoin");
      return;
    }
    gState->pulsesPerCoin = value;
  }

  if (doc.containsKey("availableCoins")) {
    int value = doc["availableCoins"];
    if (value < 0) {
      sendJsonError(400, "invalid availableCoins");
      return;
    }
    gState->availableCoins = value;
    StorageService::saveCredits(*gState);
  }

  StorageService::saveConfig(*gState);
  LogService::info(*gState, "Configuration mise à jour.");
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleStop() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(128);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::stopPost(*gState, doc["post_id"] | "", error);

  if (!ok) {
    int code = error == "post not found" ? 404 : 502;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleRecoveryResume() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(128);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::resumeInterruptedSession(*gState,
                                                   doc["post_id"] | "",
                                                   error);
  if (!ok) {
    int code = 400;
    if (error == "post not found") code = 404;
    else if (error == "no recovery pending") code = 409;
    else if (error == "poste unreachable") code = 502;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleRecoveryCancel() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(128);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::cancelInterruptedSession(*gState,
                                                   doc["post_id"] | "",
                                                   error);
  if (!ok) {
    int code = 400;
    if (error == "post not found") code = 404;
    else if (error == "no recovery pending") code = 409;
    else if (error == "poste unreachable") code = 502;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleWifiReset() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  WifiProvisioning::clearCredentials();
  LogService::warn(*gState, "Reset Wi-Fi demandé.");
  gServer->send(200, "application/json", "{\"ok\":true}");
  delay(1000);
  ESP.restart();
}

static void handleChangePassword() {
  if (!AuthService::isAuthenticated(*gServer, *gState)) {
    sendJsonError(401, "unauthorized");
    return;
  }
  if (!AuthService::isAdmin(*gServer, *gState)) {
    sendJsonError(403, "admin required");
    return;
  }

  UserAccount* currentUser = AuthService::currentUser(*gServer, *gState);
  if (!currentUser) {
    sendJsonError(401, "unauthorized");
    return;
  }
  String username = currentUser->username;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String password = doc["password"] | "";
  String error;
  if (!AuthService::changePassword(*gState, username, password, error)) {
    sendJsonError(400, error);
    return;
  }

  LogService::info(*gState, "Mot de passe modifié: " + username);
  gServer->sendHeader("Set-Cookie", "ESPSESSION=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  gServer->send(200, "application/json", "{\"ok\":true,\"reauthenticate\":true}");
}

static void handleRegenerateToken() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  String newToken = AuthService::regenerateApiToken(*gState);
  StorageService::saveAuth(*gState);
  LogService::warn(*gState, "Token API régénéré.");

  DynamicJsonDocument doc(256);
  doc["ok"] = true;
  doc["apiToken"] = newToken;

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleExportConfig() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  String configJson;
  StorageService::exportConfigJson(*gState, configJson);

  DynamicJsonDocument doc(8192);
  doc["ok"] = true;
  doc["configJson"] = configJson;

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleImportConfig() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(8192);
  if (!parseJsonBody(doc)) return;

  String configJson = doc["configJson"] | "";
  if (configJson.isEmpty()) {
    sendJsonError(400, "missing configJson");
    return;
  }

  String error;
  bool ok = StorageService::importConfigJson(*gState, configJson, error);
  if (!ok) {
    sendJsonError(400, error);
    return;
  }

  LogService::warn(*gState, "Configuration importée.");
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleLogs() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("logs");

  for (const auto& log : gState->logs) {
    JsonObject item = arr.createNestedObject();
    item["ts"] = log.ts;
    item["level"] = log.level;
    item["message"] = log.message;
  }

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleLogsClear() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  LogService::clear(*gState);
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static String currentActorUsername() {
  UserAccount* user = AuthService::currentUser(*gServer, *gState);
  return user ? user->username : String("");
}

static int userErrorStatus(const String& error) {
  if (error == "user not found") return 404;
  if (error == "username already exists") return 409;
  if (error == "cannot delete own account" ||
      error == "cannot change own role" ||
      error == "last admin required") return 409;
  return 400;
}

static void handleGetUsers() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(8192);
  doc["currentUsername"] = currentActorUsername();
  JsonArray users = doc.createNestedArray("users");

  for (const auto& user : gState->users) {
    JsonObject item = users.createNestedObject();
    item["username"] = user.username;
    item["firstName"] = user.firstName;
    item["lastName"] = user.lastName;
    item["role"] = user.role;
  }

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleCreateUser() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(512);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = AuthService::createUser(*gState,
                                    doc["username"] | "",
                                    doc["firstName"] | "",
                                    doc["lastName"] | "",
                                    doc["password"] | "",
                                    doc["role"] | "user",
                                    error);
  if (!ok) {
    sendJsonError(userErrorStatus(error), error);
    return;
  }

  String username = doc["username"] | "";
  username.trim();
  username.toLowerCase();
  LogService::info(*gState, "Utilisateur créé: " + username);
  gServer->send(201, "application/json", "{\"ok\":true}");
}

static void handleUpdateUser() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(512);
  if (!parseJsonBody(doc)) return;

  String username = doc["username"] | "";
  String error;
  bool ok = AuthService::updateUser(*gState,
                                    currentActorUsername(),
                                    username,
                                    doc["firstName"] | "",
                                    doc["lastName"] | "",
                                    doc["password"] | "",
                                    doc["role"] | "user",
                                    error);
  if (!ok) {
    sendJsonError(userErrorStatus(error), error);
    return;
  }

  LogService::info(*gState, "Utilisateur modifié: " + username);
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleDeleteUser() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String username = doc["username"] | "";
  String error;
  bool ok = AuthService::deleteUser(*gState, currentActorUsername(), username, error);
  if (!ok) {
    sendJsonError(userErrorStatus(error), error);
    return;
  }

  LogService::warn(*gState, "Utilisateur supprimé: " + username);
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handlePingPost() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String id = doc["id"] | "";
  Post* post = PostService::findById(*gState, id);
  if (!post) {
    sendJsonError(404, "post not found");
    return;
  }

  Post tmp = *post;
  bool ok = PosteClient::fetchStatus(tmp);

  DynamicJsonDocument res(256);
  res["ok"] = ok;

  if (ok) {
    LogService::info(*gState, "Ping manuel réussi pour " + id);
  } else {
    LogService::warn(*gState, "Ping manuel échoué pour " + id);
  }

  String json;
  serializeJson(res, json);
  gServer->send(200, "application/json", json);
}

static void handlePosteAnnounce() {
  if (!hasPosteAuth()) {
    sendJsonError(401, "unauthorized");
    return;
  }

  DynamicJsonDocument doc(512);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::handleAnnouncement(*gState,
                                            doc["chipId"] | "",
                                            doc["ip"] | "",
                                            doc["configured"] | false,
                                            doc["id"] | "",
                                            doc["name"] | "",
                                            doc["status"] | "",
                                            doc["relay"] | false,
                                            doc["remaining"] | 0,
                                            doc["recoveryPending"] | false,
                                            doc["recoveryRemaining"] | 0,
                                            error);
  if (!ok) {
    sendJsonError(400, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleConfigurePendingPost() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::configurePendingPost(*gState,
                                              doc["chipId"] | "",
                                              doc["name"] | "",
                                              error);
  if (!ok) {
    int code = error == "pending post not found" ? 404 : 400;
    if (error == "poste unreachable") code = 502;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

void WebRoutes::registerRoutes(WebServer& server, AppState& state) {
  gServer = &server;
  gState = &state;

  server.collectHeaders(AUTH_HEADERS, 2);

  server.on("/", HTTP_GET, handleAppPage);
  server.on("/config", HTTP_GET, handleAdminPage);
  server.on("/logs", HTTP_GET, handleAdminPage);
  server.on("/discover", HTTP_GET, handleAdminPage);
  server.on("/postes", HTTP_GET, handleAdminPage);
  server.on("/security", HTTP_GET, handleAdminPage);
  server.on("/users", HTTP_GET, handleAdminPage);
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/assets/login-station-v2.jpg", HTTP_GET, handleLoginStationAsset);
  server.on("/assets/game-room-logo-v4.webp", HTTP_GET, handleGameRoomLogoAsset);
  server.on("/assets/playstation-mark.png", HTTP_GET, handlePlayStationMarkAsset);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", HTTP_POST, handleLogout);

  server.on("/posts", HTTP_GET, handleGetPosts);
  server.on("/post/update", HTTP_POST, handleUpdatePost);
  server.on("/post/delete", HTTP_POST, handleDeletePost);
  server.on("/post/ping", HTTP_POST, handlePingPost);
  server.on("/poste/announce", HTTP_POST, handlePosteAnnounce);
  server.on("/poste/configure", HTTP_POST, handleConfigurePendingPost);
  server.on("/assign", HTTP_POST, handleAssign);
  server.on("/coins/simulate", HTTP_POST, handleSimulateCoin);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/config/export", HTTP_GET, handleExportConfig);
  server.on("/config/import", HTTP_POST, handleImportConfig);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/recovery/resume", HTTP_POST, handleRecoveryResume);
  server.on("/recovery/cancel", HTTP_POST, handleRecoveryCancel);
  server.on("/wifi/reset", HTTP_POST, handleWifiReset);
  server.on("/auth/password", HTTP_POST, handleChangePassword);
  server.on("/auth/token/regenerate", HTTP_POST, handleRegenerateToken);
  server.on("/logs/data", HTTP_GET, handleLogs);
  server.on("/logs/clear", HTTP_POST, handleLogsClear);
  server.on("/users/data", HTTP_GET, handleGetUsers);
  server.on("/users/create", HTTP_POST, handleCreateUser);
  server.on("/users/update", HTTP_POST, handleUpdateUser);
  server.on("/users/delete", HTTP_POST, handleDeleteUser);
}
