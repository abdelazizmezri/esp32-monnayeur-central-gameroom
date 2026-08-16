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

  static void loadPostsFromArray(AppState& state, JsonArray arr) {
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

      if (!p.id.isEmpty() && !p.ip.isEmpty()) {
        state.posts.push_back(p);
      }
    }
  }

  static void loadUsersFromJson(AppState& state, const String& usersJson) {
    state.users.clear();
    if (usersJson.isEmpty()) {
      return;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, usersJson);
    if (err || !doc["users"].is<JsonArray>()) {
      return;
    }

    for (JsonObject obj : doc["users"].as<JsonArray>()) {
      UserAccount user;
      user.username = obj["username"] | "";
      user.username.trim();
      user.username.toLowerCase();
      user.firstName = obj["firstName"] | "";
      user.lastName = obj["lastName"] | "";
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

      if (!duplicate && !user.username.isEmpty() && !user.firstName.isEmpty() &&
          !user.lastName.isEmpty() &&
          (user.role == "admin" || user.role == "user") &&
          !user.passwordSalt.isEmpty() && user.passwordHash.length() == 64) {
        state.users.push_back(user);
      }
    }
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

    for (const auto& user : state.users) {
      JsonObject obj = users.createNestedObject();
      obj["username"] = user.username;
      obj["firstName"] = user.firstName;
      obj["lastName"] = user.lastName;
      obj["role"] = user.role;
      obj["passwordSalt"] = user.passwordSalt;
      obj["passwordHash"] = user.passwordHash;
    }

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
    DynamicJsonDocument doc(6144);

    doc["coinDurationSeconds"] = state.coinDurationSeconds;
    doc["pulsesPerCoin"] = state.pulsesPerCoin;
    doc["availableCoins"] = state.availableCoins;
    doc["apiToken"] = state.apiToken;

    outputJson = "";
    serializeJsonPretty(doc, outputJson);
    return true;
  }

  bool importConfigJson(AppState& state, const String& inputJson, String& error) {
    DynamicJsonDocument doc(6144);
    DeserializationError err = deserializeJson(doc, inputJson);
    if (err) {
      error = "invalid json";
      return false;
    }

    if (!doc["coinDurationSeconds"].isNull()) {
      int value = doc["coinDurationSeconds"].as<int>();
      if (value <= 0) {
        error = "invalid coinDurationSeconds";
        return false;
      }
      state.coinDurationSeconds = value;
    }

    if (!doc["pulsesPerCoin"].isNull()) {
      int value = doc["pulsesPerCoin"].as<int>();
      if (value <= 0) {
        error = "invalid pulsesPerCoin";
        return false;
      }
      state.pulsesPerCoin = value;
    }

    if (!doc["availableCoins"].isNull()) {
      int value = doc["availableCoins"].as<int>();
      if (value < 0) {
        error = "invalid availableCoins";
        return false;
      }
      state.availableCoins = value;
    }

    if (!doc["apiToken"].isNull()) {
      String value = doc["apiToken"].as<String>();
      if (value.length() < 8) {
        error = "invalid apiToken";
        return false;
      }
      state.apiToken = value;
    }

    saveConfig(state);
    saveCredits(state);
    saveAuth(state);
    return true;
  }
}
