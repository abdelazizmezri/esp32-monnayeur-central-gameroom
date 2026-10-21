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
#include "RevenueService.h"
#include "CoinService.h"

#include <ArduinoJson.h>

static WebServer* gServer = nullptr;
static AppState* gState = nullptr;
static const char* AUTH_HEADERS[] = {"Authorization", "Cookie"};

static void sendHtmlPage(const char* page) {
  // Les pages embarquées sont trop grandes pour être copiées dans un String.
  // send_P les transmet directement depuis la mémoire flash, par petits blocs.
  gServer->send_P(200,
                  PSTR("text/html; charset=utf-8"),
                  reinterpret_cast<PGM_P>(page));
}

static void sendJsonError(int code, const String& message) {
  DynamicJsonDocument doc(256);
  doc["error"] = message;
  String json;
  serializeJson(doc, json);
  gServer->send(code, "application/json", json);
}

static bool parseJsonBody(JsonDocument& doc) {
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

  sendHtmlPage(WebPage::dashboardPage());
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

  sendHtmlPage(WebPage::dashboardPage());
}

static void handleLoginPage() {
  gServer->sendHeader("Cache-Control", "no-store");
  sendHtmlPage(WebPage::loginPage());
}

static void handleLoginStationAsset() {
  gServer->sendHeader("Cache-Control", "public, max-age=31536000, immutable");
  gServer->send_P(200,
                  "image/webp",
                  reinterpret_cast<PGM_P>(WebAssets::LOGIN_STATION_WEBP),
                  WebAssets::LOGIN_STATION_WEBP_LENGTH);
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
                  "image/webp",
                  reinterpret_cast<PGM_P>(WebAssets::PLAYSTATION_MARK_WEBP),
                  WebAssets::PLAYSTATION_MARK_WEBP_LENGTH);
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
    p["piecesRequired"] = post.piecesRequired;
    p["status"] = post.status;
    p["relay"] = post.relay;
    p["remaining"] = post.remaining;
    p["recoveryPending"] = post.recoveryPending;
    p["recoveryRemaining"] = post.recoveryRemaining;
  }

  JsonArray pendingArr = doc.createNestedArray("pendingPosts");
  bool adminAccess = AuthService::hasAdminAccess(*gServer, *gState);
  if (adminAccess) {
    doc["assignmentRecoveryPending"] = gState->assignmentRecoveryPending;
    for (auto& pending : gState->pendingPosts) {
      JsonObject p = pendingArr.createNestedObject();
      p["chipId"] = pending.chipId;
      p["ip"] = pending.ip;
      p["lastSeen"] = pending.lastSeen;
    }
  }

  doc["availablePieces"] = gState->availablePieces;
  doc["coinDurationSeconds"] = gState->coinDurationSeconds;
  doc["coinCurrency"] = Money::code(gState->coinCurrency);
  doc["pulseValueMinor"] = gState->pulseValueMinor;
  doc["dzdToTndRateMicros"] = gState->dzdToTndRateMicros;
  if (gState->coinCurrency == Money::TND) doc["pulseValueMillimes"] = gState->pulseValueMinor;
  doc["creditUnit"] = "pulse";
  doc["assignmentsReady"] = gState->creditStorageReady && gState->revenueStorageReady && !gState->revenueTransactionPending;
  doc["creditStorageError"] = gState->creditStorageError;
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
    doc["creditStorageReady"] = gState->creditStorageReady;
    doc["revenueTransactionPending"] = gState->revenueTransactionPending;
    doc["revenueStorageReady"] = gState->revenueStorageReady;
    doc["revenueTimeReady"] = gState->revenueTimeReady;
    doc["revenueStorageError"] = gState->revenueStorageError;
  }

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleUpdatePost() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  DynamicJsonDocument doc(256);
  if (!parseJsonBody(doc)) return;

  if (!doc["piecesRequired"].is<int>() || doc["piecesRequired"].as<int>() <= 0) {
    sendJsonError(400, "Nombre de pièces requis invalide."); return;
  }
  String error;
  bool ok = PostService::updatePost(*gState, doc["id"] | "", doc["name"] | "", doc["piecesRequired"].as<int>(), error);

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
  if (!CoinService::settle(*gState)) {
    sendJsonError(503,"Comptage en attente de sauvegarde ; réessayez après rétablissement du stockage."); return;
  }
  // Une requête = une durée. Le coût provient exclusivement du tarif du poste.
  if (doc.containsKey("coins") && (!doc["coins"].is<int>() || doc["coins"].as<int>() != 1)) {
    sendJsonError(400, "Une affectation ajoute une seule durée de jeu."); return;
  }
  bool ok = PostService::assignCoins(*gState, doc["post_id"] | "", 1, error);

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

  if (!CoinService::settle(*gState) || gState->availablePieces == INT32_MAX) {
    sendJsonError(503,"Crédits indisponibles ou solde maximal atteint."); return;
  }
  gState->availablePieces += 1;
  if (!StorageService::saveCredits(*gState)) {
    --gState->availablePieces;
    gState->creditStorageReady = false;
    sendJsonError(503,"Échec d'enregistrement des crédits NVS."); return;
  }
  LogService::info(*gState, "Simulation monnayeur: +1 pièce (sans recette), total=" + String(gState->availablePieces));

  DynamicJsonDocument doc(128);
  doc["ok"] = true;
  doc["availablePieces"] = gState->availablePieces;

  String json;
  serializeJson(doc, json);
  gServer->send(200, "application/json", json);
}

static void handleResolveAssignment() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;
  JsonDocument doc;
  if (!parseJsonBody(doc)) return;
  if (!doc["debit"].is<bool>()) { sendJsonError(400, "Choisir explicitement de confirmer ou annuler le débit."); return; }
  String error;
  if (!StorageService::resolveAssignment(*gState, doc["debit"].as<bool>(), error)) {
    sendJsonError(gState->assignmentRecoveryPending ? 503 : 400, error); return;
  }
  LogService::warn(*gState, doc["debit"].as<bool>() ? "Débit interrompu confirmé par l'administrateur." :
    "Débit interrompu annulé par l'administrateur après vérification du poste.");
  gServer->send(200, "application/json", "{\"ok\":true}");
}

static void handleConfig() {
  if (!AuthService::requireAdminAuth(*gServer,*gState)) return;
  DynamicJsonDocument doc(1024);
  if (!parseJsonBody(doc)) return;
  AppState next = *gState;
  if (doc.containsKey("coinDurationSeconds")) {
    if (!doc["coinDurationSeconds"].is<int>() || doc["coinDurationSeconds"].as<int>() <= 0) {
      sendJsonError(400,"Durée de crédit invalide."); return;
    }
    next.coinDurationSeconds = doc["coinDurationSeconds"].as<int>();
  }
  if (doc.containsKey("pulsesPerCoin") || doc.containsKey("availableCoins")) {
    sendJsonError(400,"Configurer le tarif de chaque poste et utiliser le solde availablePieces."); return;
  }
  if (doc.containsKey("coinCurrency")) {
    String currency = doc["coinCurrency"] | "";
    if (currency != "TND" && currency != "DZD") { sendJsonError(400,"Devise invalide."); return; }
    next.coinCurrency = currency == "DZD" ? Money::DZD : Money::TND;
    if (next.coinCurrency != gState->coinCurrency && !doc.containsKey("pulseValueMinor")) {
      sendJsonError(400,"Choisir la valeur de l'impulsion dans la nouvelle devise."); return;
    }
  }
  if (doc.containsKey("pulseValueMillimes")) {
    if (next.coinCurrency != Money::TND || !doc["pulseValueMillimes"].is<uint32_t>()) {
      sendJsonError(400,"pulseValueMillimes est réservé au TND."); return;
    }
    next.pulseValueMinor = doc["pulseValueMillimes"].as<uint32_t>();
  }
  if (doc.containsKey("pulseValueMinor")) {
    if (!doc["pulseValueMinor"].is<uint32_t>()) { sendJsonError(400,"Valeur monétaire invalide."); return; }
    if (doc.containsKey("pulseValueMillimes") &&
        next.pulseValueMinor != doc["pulseValueMinor"].as<uint32_t>()) {
      sendJsonError(400,"Valeurs monétaires contradictoires."); return;
    }
    next.pulseValueMinor = doc["pulseValueMinor"].as<uint32_t>();
  }
  if (doc.containsKey("dzdToTndRateMicros")) {
    if (!doc["dzdToTndRateMicros"].is<uint32_t>() ||
        !Money::validRate(doc["dzdToTndRateMicros"].as<uint32_t>())) {
      sendJsonError(400,"Taux invalide : 1 DZD = X TND, de 0,000001 à 1000."); return;
    }
    next.dzdToTndRateMicros = doc["dzdToTndRateMicros"].as<uint32_t>();
  }
  if (!Money::validValue(next.coinCurrency,next.pulseValueMinor) ||
      (next.coinCurrency == Money::DZD && !Money::validRate(next.dzdToTndRateMicros))) {
    sendJsonError(400,"Choisir une valeur de pièce valide et renseigner un taux positif avant d'activer le DZD."); return;
  }
  if (doc.containsKey("availablePieces") &&
      (!doc["availablePieces"].is<int>() || doc["availablePieces"].as<int>() < 0)) {
    sendJsonError(400,"Solde invalide."); return;
  }
  if (!CoinService::settle(*gState)) {
    sendJsonError(503,"Sauvegarde des encaissements en attente ; configuration non modifiée."); return;
  }
  // settle a pu créditer des impulsions : ne jamais restaurer un ancien solde.
  next.availablePieces = doc.containsKey("availablePieces") ? doc["availablePieces"].as<int>() : gState->availablePieces;
  next.lastRevenueSequence = gState->lastRevenueSequence;
  next.legacyRevenueSequence = gState->legacyRevenueSequence;
  if (!StorageService::saveConfig(next) || !StorageService::saveCredits(next)) {
    gState->creditStorageReady = false;
    sendJsonError(503,"Échec NVS ; redémarrer pour recharger le dernier instantané enregistré."); return;
  }
  gState->coinDurationSeconds = next.coinDurationSeconds;
  gState->coinCurrency = next.coinCurrency; gState->pulseValueMinor = next.pulseValueMinor;
  gState->dzdToTndRateMicros = next.dzdToTndRateMicros; gState->availablePieces = next.availablePieces;
  CoinService::publishConfiguration(*gState);
  LogService::info(*gState,"Configuration enregistrée (" + String(Money::code(next.coinCurrency)) + ").");
  gServer->send(200,"application/json","{\"ok\":true}");
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

  if (!CoinService::settle(*gState)) {
    sendJsonError(503,"Encaissements en attente ; sauvegarde de configuration temporairement indisponible."); return;
  }
  String configJson;
  if (!StorageService::exportConfigJson(*gState, configJson)) {
    sendJsonError(503, "Sauvegarde trop volumineuse ou mémoire insuffisante."); return;
  }
  gServer->sendHeader("Cache-Control", "no-store");
  gServer->send(200, "application/json", configJson);
}

static void handleImportConfig() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  if (!gServer->hasArg("plain") || gServer->arg("plain").isEmpty()) {
    sendJsonError(400, "missing backup");
    return;
  }

  String error;
  if (!CoinService::settle(*gState)) {
    sendJsonError(503,"Encaissements en attente de sauvegarde ; restauration refusée."); return;
  }
  bool ok = StorageService::importConfigJson(*gState, gServer->arg("plain"), error);
  if (!ok) {
    sendJsonError(400, error);
    return;
  }

  CoinService::publishConfiguration(*gState);
  LogService::warn(*gState, "Sauvegarde complète restaurée.");
  gServer->sendHeader("Set-Cookie", "ESPSESSION=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  gServer->send(200, "application/json", "{\"ok\":true,\"reauthenticate\":true}");
}

static bool buildRevenueReport(RevenueReport& report) {
  String period = gServer->arg("period");
  if (period.isEmpty()) period = "day";
  String error;
  if (!RevenueService::buildReport(*gState, period, gServer->arg("anchor"),
                                   report, error)) {
    sendJsonError(400, error);
    return false;
  }
  return true;
}

static void handleRevenueData() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  RevenueReport report;
  if (!buildRevenueReport(report)) return;

  DynamicJsonDocument doc(16384);
  doc["period"] = report.period;
  doc["rangeLabel"] = report.rangeLabel;
  doc["startDate"] = report.startDate;
  doc["endDate"] = report.endDate;
  doc["generatedAt"] = report.generatedAt;
  doc["storageReady"] = report.storageReady;
  doc["timeReady"] = report.timeReady;
  doc["storageError"] = gState->revenueStorageError;
  doc["fileBytes"] = report.fileBytes;
  doc["storageUsedBytes"] = report.storageUsedBytes;
  doc["storageTotalBytes"] = report.storageTotalBytes;
  doc["totalMillimes"] = report.totalMillimes;
  doc["totalTndMillimes"] = report.totalTndMillimes;
  doc["totalDzdCentimes"] = report.totalDzdCentimes;
  doc["currentRateMicros"] = report.currentRateMicros;
  doc["conversionPolicy"] = "historical-per-pulse-half-up";
  doc["undatedMillimes"] = report.undatedMillimes;
  doc["undatedTndMillimes"] = report.undatedTndMillimes;
  doc["undatedDzdCentimes"] = report.undatedDzdCentimes;
  doc["pulses"] = report.pulses;
  doc["credits"] = report.credits;
  doc["events"] = report.events;
  doc["undatedEvents"] = report.undatedEvents;
  doc["corruptRecords"] = report.corruptRecords;
  JsonArray items = doc.createNestedArray("items");
  for (const auto& bucket : report.buckets) {
    JsonObject item = items.createNestedObject();
    item["key"] = bucket.key;
    item["label"] = bucket.label;
    item["amountMillimes"] = bucket.amountMillimes;
    item["tndMillimes"] = bucket.tndMillimes;
    item["dzdCentimes"] = bucket.dzdCentimes;
    item["pulses"] = bucket.pulses;
    item["credits"] = bucket.credits;
    item["events"] = bucket.events;
  }

  String json;
  serializeJson(doc, json);
  gServer->sendHeader("Cache-Control", "no-store");
  gServer->send(200, "application/json", json);
}

static void handleRevenueCsv() {
  if (!AuthService::requireAdminAuth(*gServer, *gState)) return;

  RevenueReport report;
  if (!buildRevenueReport(report)) return;
  String filename = "recette-" + report.period + "-" + report.startDate + ".csv";
  gServer->sendHeader("Cache-Control", "no-store");
  gServer->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  gServer->send(200, "text/csv; charset=utf-8", RevenueService::buildCsv(report));
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

  int piecesRequired = AppConfig::DEFAULT_PIECES_REQUIRED;
  if (doc.containsKey("piecesRequired")) {
    if (!doc["piecesRequired"].is<int>() || doc["piecesRequired"].as<int>() <= 0) {
      sendJsonError(400, "Nombre de pièces requis invalide."); return;
    }
    piecesRequired = doc["piecesRequired"].as<int>();
  }
  String error;
  bool ok = PostService::configurePendingPost(*gState,
                                              doc["chipId"] | "",
                                              doc["name"] | "",
                                              piecesRequired, error);
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
  server.on("/recette", HTTP_GET, handleAdminPage);
  server.on("/logs", HTTP_GET, handleAdminPage);
  server.on("/discover", HTTP_GET, handleAdminPage);
  server.on("/postes", HTTP_GET, handleAdminPage);
  server.on("/security", HTTP_GET, handleAdminPage);
  server.on("/users", HTTP_GET, handleAdminPage);
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/assets/login-station-v4.webp", HTTP_GET, handleLoginStationAsset);
  server.on("/assets/game-room-logo-v5.webp", HTTP_GET, handleGameRoomLogoAsset);
  server.on("/assets/playstation-mark-v2.webp", HTTP_GET, handlePlayStationMarkAsset);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", HTTP_POST, handleLogout);

  server.on("/posts", HTTP_GET, handleGetPosts);
  server.on("/post/update", HTTP_POST, handleUpdatePost);
  server.on("/post/delete", HTTP_POST, handleDeletePost);
  server.on("/post/ping", HTTP_POST, handlePingPost);
  server.on("/poste/announce", HTTP_POST, handlePosteAnnounce);
  server.on("/poste/configure", HTTP_POST, handleConfigurePendingPost);
  server.on("/assign", HTTP_POST, handleAssign);
  server.on("/pieces/simulate", HTTP_POST, handleSimulateCoin);
  server.on("/assignment/resolve", HTTP_POST, handleResolveAssignment);
  server.on("/coins/simulate", HTTP_POST, handleSimulateCoin); // Alias historique : +1 pièce, sans recette.
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/config/export", HTTP_GET, handleExportConfig);
  server.on("/config/import", HTTP_POST, handleImportConfig);
  server.on("/revenue/data", HTTP_GET, handleRevenueData);
  server.on("/revenue/export.csv", HTTP_GET, handleRevenueCsv);
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
