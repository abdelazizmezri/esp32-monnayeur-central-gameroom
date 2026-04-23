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
      obj["id"] = post.id;
      obj["name"] = post.name;
      obj["ip"] = post.ip;
    }
  }

  void load(AppState& state) {
    prefs.begin("appcfg", true);
    state.coinDurationSeconds = prefs.getInt("coinDur", AppConfig::DEFAULT_COIN_DURATION_SECONDS);
    state.pulsesPerCoin = prefs.getInt("pulseCoin", AppConfig::DEFAULT_PULSES_PER_COIN);
    state.availableCoins = prefs.getInt("availCoins", AppConfig::DEFAULT_AVAILABLE_COINS);
    prefs.end();

    prefs.begin("auth", true);
    state.adminPassword = prefs.getString("adminPwd", AppConfig::DEFAULT_ADMIN_PASSWORD);
    state.apiToken = prefs.getString("apiToken", "");
    prefs.end();

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
    if (!err && doc.is<JsonArray>()) {
      state.posts.clear();

      for (JsonObject obj : doc.as<JsonArray>()) {
        Post p;
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
    JsonArray arr = doc.to<JsonArray>();
    buildPostsArray(arr, state);

    String json;
    serializeJson(doc, json);

    prefs.begin("posts", false);
    prefs.putString("items", json);
    prefs.end();
  }

  void saveAuth(const AppState& state) {
    prefs.begin("auth", false);
    prefs.putString("adminPwd", state.adminPassword);
    prefs.putString("apiToken", state.apiToken);
    prefs.end();
  }

  bool exportConfigJson(const AppState& state, String& outputJson) {
    DynamicJsonDocument doc(6144);

    doc["coinDurationSeconds"] = state.coinDurationSeconds;
    doc["pulsesPerCoin"] = state.pulsesPerCoin;
    doc["availableCoins"] = state.availableCoins;
    doc["adminPassword"] = state.adminPassword;
    doc["apiToken"] = state.apiToken;

    JsonArray posts = doc.createNestedArray("posts");
    buildPostsArray(posts, state);

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

    if (!doc["adminPassword"].isNull()) {
      String value = doc["adminPassword"].as<String>();
      if (value.length() < 4) {
        error = "invalid adminPassword";
        return false;
      }
      state.adminPassword = value;
    }

    if (!doc["apiToken"].isNull()) {
      String value = doc["apiToken"].as<String>();
      if (value.length() < 8) {
        error = "invalid apiToken";
        return false;
      }
      state.apiToken = value;
    }

    if (doc["posts"].is<JsonArray>()) {
      std::vector<Post> newPosts;

      for (JsonObject obj : doc["posts"].as<JsonArray>()) {
        String id = obj["id"] | "";
        String name = obj["name"] | "";
        String ip = obj["ip"] | "";
        id.trim();
        name.trim();
        ip.trim();

        if (id.isEmpty() || name.isEmpty() || ip.isEmpty()) {
          error = "invalid post entry";
          return false;
        }

        for (const auto& existing : newPosts) {
          if (existing.id == id) {
            error = "duplicate post id in import";
            return false;
          }
          if (existing.ip == ip) {
            error = "duplicate post ip in import";
            return false;
          }
        }

        Post p;
        p.id = id;
        p.name = name;
        p.ip = ip;
        p.status = "idle";
        p.relay = false;
        p.remaining = 0;
        p.lastSeen = 0;
        newPosts.push_back(p);
      }

      state.posts = newPosts;
    }

    saveConfig(state);
    saveCredits(state);
    savePosts(state);
    saveAuth(state);
    return true;
  }
}
