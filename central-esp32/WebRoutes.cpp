#include "WebRoutes.h"
#include "WebPage.h"
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

static void handleRoot() {
  if (!AuthService::isAuthenticated(*gServer, *gState)) {
    gServer->sendHeader("Location", "/login", true);
    gServer->send(302, "text/plain", "");
    return;
  }

  gServer->send(200, "text/html", WebPage::dashboardPage());
}

static void handleLoginPage() {
  gServer->send(200, "text/html", WebPage::loginPage());
}

static void handleLogin() {
  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String password = doc["password"] | "";
  if (!AuthService::login(*gState, password)) {
    sendJsonError(401, "Mot de passe incorrect");
    return;
  }

  LogService::info(*gState, "Connexion admin réussie.");
  gServer->sendHeader("Set-Cookie", "ESPSESSION=" + gState->sessionToken + "; Path=/; HttpOnly");
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleLogout() {
  AuthService::logout(*gState);
  gServer->sendHeader("Set-Cookie", "ESPSESSION=; Path=/; Max-Age=0");
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleGetPosts() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("posts");

  for (auto& post : gState->posts) {
    JsonObject p = arr.createNestedObject();
    p["id"] = post.id;
    p["name"] = post.name;
    p["ip"] = post.ip;
    p["status"] = post.status;
    p["relay"] = post.relay;
    p["remaining"] = post.remaining;
  }

  doc["availableCoins"] = gState->availableCoins;
  doc["coinDurationSeconds"] = gState->coinDurationSeconds;
  doc["pulsesPerCoin"] = gState->pulsesPerCoin;
  doc["apiTokenMasked"] = AuthService::getMaskedToken(*gState);

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleAddPost() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::addPost(*gState, doc["id"] | "", doc["name"] | "", doc["ip"] | "", error);

  if (!ok) {
    sendJsonError(400, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleUpdatePost() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::updatePost(*gState, doc["id"] | "", doc["name"] | "", doc["ip"] | "", error);

  if (!ok) {
    int code = error == "post not found" ? 404 : 400;
    sendJsonError(code, error);
    return;
  }

  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleDeletePost() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(128);
  if (!parseJsonBody(doc)) return;

  String error;
  bool ok = PostService::deletePost(*gState, doc["id"] | "", error);

  if (!ok) {
    sendJsonError(404, error);
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
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

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
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

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

static void handleWifiReset() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  WifiProvisioning::clearCredentials();
  LogService::warn(*gState, "Reset Wi-Fi demandé.");
  gServer->send(200, "application/json", "{\"ok\":true}");
  delay(1000);
  ESP.restart();
}

static void handleChangePassword() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  String password = doc["password"] | "";
  if (password.length() < 4) {
    sendJsonError(400, "Mot de passe trop court");
    return;
  }

  gState->adminPassword = password;
  StorageService::saveAuth(*gState);
  LogService::info(*gState, "Mot de passe admin modifié.");
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleRegenerateToken() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

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
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

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
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

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
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

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
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

  LogService::clear(*gState);
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handlePingPost() {
  if (!AuthService::requireApiAuth(*gServer, *gState)) return;

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

void WebRoutes::registerRoutes(WebServer& server, AppState& state) {
  gServer = &server;
  gState = &state;

  server.collectHeaders(AUTH_HEADERS, 2);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", HTTP_POST, handleLogout);

  server.on("/posts", HTTP_GET, handleGetPosts);
  server.on("/add-post", HTTP_POST, handleAddPost);
  server.on("/post/update", HTTP_POST, handleUpdatePost);
  server.on("/post/delete", HTTP_POST, handleDeletePost);
  server.on("/post/ping", HTTP_POST, handlePingPost);
  server.on("/assign", HTTP_POST, handleAssign);
  server.on("/coins/simulate", HTTP_POST, handleSimulateCoin);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/config/export", HTTP_GET, handleExportConfig);
  server.on("/config/import", HTTP_POST, handleImportConfig);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/wifi/reset", HTTP_POST, handleWifiReset);
  server.on("/auth/password", HTTP_POST, handleChangePassword);
  server.on("/auth/token/regenerate", HTTP_POST, handleRegenerateToken);
  server.on("/logs", HTTP_GET, handleLogs);
  server.on("/logs/clear", HTTP_POST, handleLogsClear);
}
