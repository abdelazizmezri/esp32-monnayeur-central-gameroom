#include "StorageService.h"
#include "AppConfig.h"
#include "RevenueJournal.h"

#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_system.h>

static Preferences prefs;

namespace StorageService {
  static bool sanitizeRuntimeConfig(AppState& state) {
    bool changed = false;

    if (state.coinDurationSeconds <= 0) {
      state.coinDurationSeconds = AppConfig::DEFAULT_COIN_DURATION_SECONDS;
      changed = true;
    }

    if (state.pulsesPerCoin <= 0) {
      state.pulsesPerCoin = AppConfig::DEFAULT_PULSES_PER_COIN;
      changed = true;
    }

    if (!Money::validValue(state.coinCurrency, state.pulseValueMinor) ||
        (state.coinCurrency == Money::DZD && !Money::validRate(state.dzdToTndRateMicros)) ||
        state.dzdToTndRateMicros > Money::MAX_RATE) {
      state.coinCurrency = Money::TND;
      state.pulseValueMinor = AppConfig::DEFAULT_PULSE_VALUE_MILLIMES;
      state.dzdToTndRateMicros = 0;
      changed = true;
    }

    if (state.availableCoins < 0) {
      state.availableCoins = AppConfig::DEFAULT_AVAILABLE_COINS;
      changed = true;
    }

    if (state.pendingPulsesPerCoin <= 0 || state.pendingPulses < 0 || state.pendingPulses >= state.pendingPulsesPerCoin) {
      state.pendingPulses = 0;
      state.pendingPulsesPerCoin = state.pulsesPerCoin;
      changed = true;
    }

    return changed;
  }

  static String generateDefaultToken() {
    uint32_t a = (uint32_t)esp_random();
    uint32_t b = (uint32_t)esp_random();
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%08lx%08lx", (unsigned long)a, (unsigned long)b);
    return String(buffer);
  }

  static void buildPostsArray(JsonArray arr, const AppState& state) {
    for (const auto& post : state.posts) {
      JsonObject obj = arr.createNestedObject();
      obj["chipId"] = post.chipId;
      obj["id"] = post.id;
      obj["name"] = post.name;
      obj["ip"] = post.ip;
    }
  }

  static void buildUsersArray(JsonArray arr, const AppState& state) {
    for (const auto& user : state.users) {
      JsonObject obj = arr.createNestedObject();
      obj["username"] = user.username;
      obj["firstName"] = user.firstName;
      obj["lastName"] = user.lastName;
      obj["role"] = user.role;
      obj["passwordSalt"] = user.passwordSalt;
      obj["passwordHash"] = user.passwordHash;
    }
  }

  static bool validUsername(const String& username) {
    if (username.length() < 3 || username.length() > 32) return false;
    for (size_t i = 0; i < username.length(); i++) {
      char c = username.charAt(i);
      if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '.' || c == '_' || c == '-')) return false;
    }
    return true;
  }

  static bool loadPostsFromArray(AppState& state, JsonArray arr) {
    state.posts.clear();

    for (JsonObject obj : arr) {
      Post p;
      p.chipId = obj["chipId"] | "";
      p.id = obj["id"] | "";
      p.name = obj["name"] | "";
      p.ip = obj["ip"] | "";
      p.status = "idle";
      p.relay = false;
      p.remaining = 0;
      p.lastSeen = 0;

      bool duplicate = false;
      for (const auto& existing : state.posts) {
        if (existing.id == p.id || existing.ip == p.ip ||
            (!p.chipId.isEmpty() && existing.chipId == p.chipId)) {
          duplicate = true;
          break;
        }
      }

      if (duplicate || p.id.isEmpty() || p.name.isEmpty() || p.ip.isEmpty()) return false;
      state.posts.push_back(p);
    }
    return true;
  }

  static bool loadUsersFromArray(AppState& state, JsonArray arr) {
    state.users.clear();
    if (arr.size() == 0 || arr.size() > 10) return false;
    bool hasAdmin = false;

    for (JsonObject obj : arr) {
      UserAccount user;
      user.username = obj["username"] | "";
      user.username.trim();
      user.username.toLowerCase();
      user.firstName = obj["firstName"] | "";
      user.firstName.trim();
      user.lastName = obj["lastName"] | "";
      user.lastName.trim();
      user.role = obj["role"] | "user";
      user.passwordSalt = obj["passwordSalt"] | "";
      user.passwordHash = obj["passwordHash"] | "";

      bool duplicate = false;
      for (const auto& existing : state.users) {
        if (existing.username == user.username) {
          duplicate = true;
          break;
        }
      }

      if (duplicate || !validUsername(user.username) || user.firstName.isEmpty() ||
          user.lastName.isEmpty() || user.firstName.length() > 64 ||
          user.lastName.length() > 64 ||
          (user.role != "admin" && user.role != "user") ||
          user.passwordSalt.isEmpty() || user.passwordHash.length() != 64) return false;

      state.users.push_back(user);
      hasAdmin |= user.role == "admin";
    }
    return hasAdmin;
  }

  static void loadUsersFromJson(AppState& state, const String& usersJson) {
    if (usersJson.isEmpty()) return;
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, usersJson);
    if (err || !doc["users"].is<JsonArray>() ||
        !loadUsersFromArray(state, doc["users"].as<JsonArray>())) state.users.clear();
  }

  void load(AppState& state) {
    prefs.begin("appcfg", true);
    state.coinDurationSeconds = prefs.getInt("coinDur", AppConfig::DEFAULT_COIN_DURATION_SECONDS);
    state.pulsesPerCoin = prefs.getInt("pulseCoin", AppConfig::DEFAULT_PULSES_PER_COIN);
    state.pulseValueMinor = prefs.getInt("pulseVal", AppConfig::DEFAULT_PULSE_VALUE_MILLIMES);
    state.availableCoins = prefs.getInt("availCoins", AppConfig::DEFAULT_AVAILABLE_COINS);
    state.pendingPulses = prefs.getInt("pendingPulse", 0);
    state.pendingPulsesPerCoin = state.pulsesPerCoin;
    prefs.end();

    // Une seule valeur NVS atomique par instantané, avec CRC et relecture.
    prefs.begin("appcfg", true);
    uint8_t config[32] = {}, credits[40] = {};
    bool hasConfig = prefs.isKey("configV3");
    bool hasCredits = prefs.isKey("creditsV3");
    if (hasConfig) {
      if (prefs.getBytesLength("configV3") != sizeof(config) ||
          prefs.getBytes("configV3", config, sizeof(config)) != sizeof(config) ||
          memcmp(config, "CFG3", 4) ||
          RevenueJournal::get(config+30,2) != Money::crc16(config,30)) {
        state.creditStorageReady = false;
      } else {
        state.coinDurationSeconds = RevenueJournal::get(config+4,4);
        state.pulsesPerCoin = RevenueJournal::get(config+8,4);
        state.coinCurrency = config[12];
        state.pulseValueMinor = RevenueJournal::get(config+16,4);
        state.dzdToTndRateMicros = RevenueJournal::get(config+20,4);
      }
    }
    if (hasCredits) {
      if (prefs.getBytesLength("creditsV3") != sizeof(credits) ||
          prefs.getBytes("creditsV3", credits, sizeof(credits)) != sizeof(credits) ||
          memcmp(credits,"CRD3",4) ||
          RevenueJournal::get(credits+38,2) != Money::crc16(credits,38)) {
        state.creditStorageReady = false;
      } else {
        state.availableCoins = RevenueJournal::get(credits+4,4);
        state.pendingPulses = RevenueJournal::get(credits+8,4);
        state.pendingPulsesPerCoin = RevenueJournal::get(credits+12,4);
        state.lastRevenueSequence = RevenueJournal::get(credits+16,8);
      }
    }
    prefs.end();
    if (!state.creditStorageReady)
      state.revenueStorageError = "Instantané NVS illisible ; comptage suspendu pour protéger les données.";

    prefs.begin("auth", true);
    state.legacyAdminPassword = prefs.getString("adminPwd", "");
    state.apiToken = prefs.getString("apiToken", "");
    String usersJson = prefs.getString("users", "");
    prefs.end();

    loadUsersFromJson(state, usersJson);

    if (state.apiToken.isEmpty()) {
      state.apiToken = generateDefaultToken();
      saveAuth(state);
    }

    prefs.begin("posts", true);
    String postsJson = prefs.getString("items", "[]");
    prefs.end();

    bool configChanged = sanitizeRuntimeConfig(state);

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, postsJson);
    if (!err) {
      if (doc.is<JsonArray>()) {
        loadPostsFromArray(state, doc.as<JsonArray>());
      } else if (doc.is<JsonObject>()) {
        JsonObject root = doc.as<JsonObject>();
        if (root["posts"].is<JsonArray>()) {
          loadPostsFromArray(state, root["posts"].as<JsonArray>());
        }
      }
    }

    if (state.creditStorageReady) {
      if (configChanged || !hasConfig) state.creditStorageReady = saveConfig(state);
      if (state.creditStorageReady && (configChanged || !hasCredits))
        state.creditStorageReady = saveCredits(state);
    }
  }

  static bool writeVerified(const char* ns, const char* key, const uint8_t* data, size_t size) {
    if (!prefs.begin(ns, false)) return false;
    bool ok = prefs.putBytes(key, data, size) == size;
    uint8_t check[RevenueJournal::RECORD_SIZE] = {};
    ok = ok && size <= sizeof(check) && prefs.getBytesLength(key) == size &&
         prefs.getBytes(key,check,size) == size && memcmp(data,check,size) == 0;
    prefs.end();
    return ok;
  }

  bool saveConfig(const AppState& state) {
    uint8_t data[32] = {'C','F','G','3'};
    RevenueJournal::put(data+4,state.coinDurationSeconds,4);
    RevenueJournal::put(data+8,state.pulsesPerCoin,4);
    data[12] = state.coinCurrency;
    RevenueJournal::put(data+16,state.pulseValueMinor,4);
    RevenueJournal::put(data+20,state.dzdToTndRateMicros,4);
    RevenueJournal::put(data+30,Money::crc16(data,30),2);
    return writeVerified("appcfg","configV3",data,sizeof(data));
  }

  bool saveCredits(const AppState& state) {
    uint8_t data[40] = {'C','R','D','3'};
    RevenueJournal::put(data+4,state.availableCoins,4);
    RevenueJournal::put(data+8,state.pendingPulses,4);
    RevenueJournal::put(data+12,state.pendingPulsesPerCoin,4);
    RevenueJournal::put(data+16,state.lastRevenueSequence,8);
    RevenueJournal::put(data+38,Money::crc16(data,38),2);
    return writeVerified("appcfg","creditsV3",data,sizeof(data));
  }

  int loadRevenueTransaction(RevenueJournal::Entry& entry) {
    if (!prefs.begin("revenue",true)) return 0; // namespace absent à la première installation
    if (!prefs.isKey("pending")) { prefs.end(); return 0; }
    uint8_t data[RevenueJournal::RECORD_SIZE];
    bool ok = prefs.getBytesLength("pending") == sizeof(data) &&
              prefs.getBytes("pending",data,sizeof(data)) == sizeof(data);
    prefs.end();
    return ok && RevenueJournal::decode(data,entry) && entry.sequence > 0 ? 1 : -1;
  }

  bool stageRevenueTransaction(const RevenueJournal::Entry& entry) {
    uint8_t data[RevenueJournal::RECORD_SIZE];
    RevenueJournal::encode(entry,data);
    return writeVerified("revenue","pending",data,sizeof(data));
  }

  bool clearRevenueTransaction() {
    if (!prefs.begin("revenue",false)) return false;
    bool ok = !prefs.isKey("pending") || prefs.remove("pending");
    prefs.end();
    return ok;
  }

  void savePosts(const AppState& state) {
    DynamicJsonDocument doc(4096);
    JsonArray posts = doc.createNestedArray("posts");
    buildPostsArray(posts, state);

    String json;
    serializeJson(doc, json);

    prefs.begin("posts", false);
    prefs.putString("items", json);
    prefs.end();
  }

  void saveAuth(const AppState& state) {
    prefs.begin("auth", false);
    prefs.putString("apiToken", state.apiToken);
    prefs.end();
  }

  void saveUsers(const AppState& state) {
    DynamicJsonDocument doc(8192);
    JsonArray users = doc.createNestedArray("users");
    buildUsersArray(users, state);

    String json;
    serializeJson(doc, json);

    prefs.begin("auth", false);
    size_t written = prefs.putString("users", json);
    if (written > 0) {
      prefs.remove("adminPwd");
    }
    prefs.end();
  }

  bool exportConfigJson(const AppState& state, String& outputJson) {
    DynamicJsonDocument doc(24576);

    doc["format"] = "pstm-backup";
    doc["version"] = 3;
    doc["coinDurationSeconds"] = state.coinDurationSeconds;
    doc["pulsesPerCoin"] = state.pulsesPerCoin;
    doc["coinCurrency"] = Money::code(state.coinCurrency);
    doc["pulseValueMinor"] = state.pulseValueMinor;
    doc["dzdToTndRateMicros"] = state.dzdToTndRateMicros;
    if (state.coinCurrency == Money::TND) doc["pulseValueMillimes"] = state.pulseValueMinor;
    doc["pendingPulsesPerCoin"] = state.pendingPulsesPerCoin;
    doc["availableCoins"] = state.availableCoins;
    doc["pendingPulses"] = state.pendingPulses;
    doc["apiToken"] = state.apiToken;
    buildUsersArray(doc.createNestedArray("users"), state);
    buildPostsArray(doc.createNestedArray("posts"), state);

    prefs.begin("wifi", true);
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = prefs.getString("ssid", "");
    wifi["password"] = prefs.getString("password", "");
    wifi["staticIp"] = prefs.getBool("staticIp", false);
    wifi["localIp"] = prefs.getString("localIp", "");
    wifi["gateway"] = prefs.getString("gateway", "");
    wifi["subnet"] = prefs.getString("subnet", "");
    wifi["dns1"] = prefs.getString("dns1", "");
    wifi["dns2"] = prefs.getString("dns2", "");
    prefs.end();

    outputJson = "";
    serializeJson(doc, outputJson);
    return true;
  }

  bool importConfigJson(AppState& state, const String& inputJson, String& error) {
    DynamicJsonDocument doc(24576);
    DeserializationError err = deserializeJson(doc, inputJson);
    if (err || !doc.is<JsonObject>()) {
      error = "invalid json";
      return false;
    }

    int backupVersion = doc["version"] | 0;
    if (String(doc["format"] | "") != "pstm-backup" ||
        (backupVersion < 1 || backupVersion > 3) ||
        !doc["users"].is<JsonArray>() || !doc["posts"].is<JsonArray>()) {
      error = "invalid backup";
      return false;
    }

    AppState restored;
    restored.coinDurationSeconds = doc["coinDurationSeconds"] | 0;
    restored.pulsesPerCoin = doc["pulsesPerCoin"] | 0;
    restored.pulseValueMinor = backupVersion >= 2
                                     ? (doc["pulseValueMillimes"] | 0)
                                     : AppConfig::DEFAULT_PULSE_VALUE_MILLIMES;
    if (backupVersion == 3) {
      String currency = doc["coinCurrency"] | "";
      restored.coinCurrency = currency == "TND" ? Money::TND : currency == "DZD" ? Money::DZD : 255;
      if (!doc["pulseValueMinor"].is<uint32_t>() || !doc["dzdToTndRateMicros"].is<uint32_t>()) {
        error = "invalid monetary settings"; return false;
      }
      restored.pulseValueMinor = doc["pulseValueMinor"].as<uint32_t>();
      restored.dzdToTndRateMicros = doc["dzdToTndRateMicros"].as<uint32_t>();
    }
    restored.availableCoins = doc["availableCoins"] | -1;
    restored.pendingPulses = backupVersion >= 2 ? (doc["pendingPulses"] | 0) : 0;
    restored.apiToken = doc["apiToken"] | "";
    restored.pendingPulsesPerCoin = backupVersion == 3
      ? (doc["pendingPulsesPerCoin"] | 0) : restored.pulsesPerCoin;
    bool pulseValueValid = Money::validValue(restored.coinCurrency,restored.pulseValueMinor) &&
      restored.dzdToTndRateMicros <= Money::MAX_RATE &&
      (restored.coinCurrency != Money::DZD || Money::validRate(restored.dzdToTndRateMicros));
    if (restored.coinDurationSeconds <= 0 || restored.pulsesPerCoin <= 0 ||
        !pulseValueValid ||
        restored.availableCoins < 0 || restored.pendingPulses < 0 ||
        restored.pendingPulsesPerCoin <= 0 || restored.pendingPulses >= restored.pendingPulsesPerCoin ||
        restored.apiToken.length() < 8) {
      error = "invalid settings";
      return false;
    }

    if (!loadUsersFromArray(restored, doc["users"].as<JsonArray>())) {
      error = "invalid users";
      return false;
    }
    if (!loadPostsFromArray(restored, doc["posts"].as<JsonArray>())) {
      error = "invalid posts";
      return false;
    }

    if (!state.creditStorageReady || state.revenueTransactionPending) {
      error = "Transaction financière en attente ; restauration refusée."; return false;
    }
    restored.lastRevenueSequence = state.lastRevenueSequence;
    if (!saveConfig(restored) || !saveCredits(restored)) {
      state.creditStorageReady = false;
      error = "Échec de sauvegarde NVS ; redémarrer avant de réessayer."; return false;
    }
    state.coinDurationSeconds = restored.coinDurationSeconds;
    state.pulsesPerCoin = restored.pulsesPerCoin;
    state.coinCurrency = restored.coinCurrency;
    state.pulseValueMinor = restored.pulseValueMinor;
    state.dzdToTndRateMicros = restored.dzdToTndRateMicros;
    state.pendingPulsesPerCoin = restored.pendingPulsesPerCoin;
    state.availableCoins = restored.availableCoins;
    state.pendingPulses = restored.pendingPulses;
    state.apiToken = restored.apiToken;
    state.users.swap(restored.users);
    state.posts.swap(restored.posts);
    state.pendingPosts.clear();
    state.sessions.clear();

    saveAuth(state);
    saveUsers(state);
    savePosts(state);

    if (doc["wifi"].is<JsonObject>()) {
      JsonObject wifi = doc["wifi"].as<JsonObject>();
      prefs.begin("wifi", false);
      prefs.putString("ssid", wifi["ssid"] | "");
      prefs.putString("password", wifi["password"] | "");
      prefs.putBool("staticIp", wifi["staticIp"] | false);
      const char* keys[] = {"localIp", "gateway", "subnet", "dns1", "dns2"};
      for (const char* key : keys) prefs.putString(key, wifi[key] | "");
      prefs.end();
    }
    return true;
  }
}
