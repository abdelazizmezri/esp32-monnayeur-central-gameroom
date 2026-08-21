#include "StorageService.h"
#include "AppConfig.h"

#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_system.h>

static Preferences prefs;

namespace StorageService {

  void begin() {
  }

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

    if (state.availableCoins < 0) {
      state.availableCoins = AppConfig::DEFAULT_AVAILABLE_COINS;
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
    state.availableCoins = prefs.getInt("availCoins", AppConfig::DEFAULT_AVAILABLE_COINS);
    prefs.end();

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

    if (configChanged) {
      saveConfig(state);
      saveCredits(state);
    }
  }

  void saveConfig(const AppState& state) {
    prefs.begin("appcfg", false);
    prefs.putInt("coinDur", state.coinDurationSeconds);
    prefs.putInt("pulseCoin", state.pulsesPerCoin);
    prefs.end();
  }

  void saveCredits(const AppState& state) {
    prefs.begin("appcfg", false);
    prefs.putInt("availCoins", state.availableCoins);
    prefs.end();
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
    doc["version"] = 1;
    doc["coinDurationSeconds"] = state.coinDurationSeconds;
    doc["pulsesPerCoin"] = state.pulsesPerCoin;
    doc["availableCoins"] = state.availableCoins;
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

    if (String(doc["format"] | "") != "pstm-backup" || (doc["version"] | 0) != 1 ||
        !doc["users"].is<JsonArray>() || !doc["posts"].is<JsonArray>()) {
      error = "invalid backup";
      return false;
    }

    AppState restored;
    restored.coinDurationSeconds = doc["coinDurationSeconds"] | 0;
    restored.pulsesPerCoin = doc["pulsesPerCoin"] | 0;
    restored.availableCoins = doc["availableCoins"] | -1;
    restored.apiToken = doc["apiToken"] | "";
    if (restored.coinDurationSeconds <= 0 || restored.pulsesPerCoin <= 0 ||
        restored.availableCoins < 0 || restored.apiToken.length() < 8) {
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

    state.coinDurationSeconds = restored.coinDurationSeconds;
    state.pulsesPerCoin = restored.pulsesPerCoin;
    state.availableCoins = restored.availableCoins;
    state.apiToken = restored.apiToken;
    state.users.swap(restored.users);
    state.posts.swap(restored.posts);
    state.pendingPosts.clear();
    state.sessions.clear();

    saveConfig(state);
    saveCredits(state);
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
