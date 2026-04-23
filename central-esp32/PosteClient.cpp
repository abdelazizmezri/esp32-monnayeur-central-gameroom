#include "PosteClient.h"
#include "AppConfig.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace PosteClient {
  static void addAuthHeader(HTTPClient& http) {
    http.addHeader("Authorization", String("Bearer ") + AppConfig::POSTE_COMMAND_TOKEN);
  }

  bool sendCommand(const String& ip, const String& action, long durationSeconds) {
    HTTPClient http;
    String url = "http://" + ip + "/command";

    http.begin(url);
    http.setTimeout(AppConfig::POSTE_STATUS_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");
    addAuthHeader(http);

    StaticJsonDocument<256> doc;
    doc["action"] = action;
    if (durationSeconds > 0) {
      doc["duration"] = durationSeconds;
    }

    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    http.end();

    return code >= 200 && code < 300;
  }

  bool configurePost(const String& ip, const String& id, const String& name) {
    HTTPClient http;
    String url = "http://" + ip + "/configure";

    http.begin(url);
    http.setTimeout(AppConfig::POSTE_STATUS_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");
    addAuthHeader(http);

    StaticJsonDocument<256> doc;
    doc["id"] = id;
    doc["name"] = name;

    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    http.end();

    return code >= 200 && code < 300;
  }

  bool clearIdentity(const String& ip) {
    HTTPClient http;
    String url = "http://" + ip + "/identity/clear";

    http.begin(url);
    http.setTimeout(AppConfig::POSTE_STATUS_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");
    addAuthHeader(http);

    int code = http.POST("{}");
    http.end();

    return code >= 200 && code < 300;
  }

  bool fetchStatus(Post& post, uint16_t timeoutMs) {
    HTTPClient http;
    String url = "http://" + post.ip + "/status";

    http.begin(url);
    http.setTimeout(timeoutMs);
    int code = http.GET();

    if (code != 200) {
      http.end();
      return false;
    }

    String payload = http.getString();
    http.end();

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      return false;
    }

    post.id = doc["id"] | post.id;
    post.chipId = doc["chipId"] | post.chipId;
    post.name = doc["name"] | post.name;
    post.status = doc["status"] | "unknown";
    post.relay = doc["relay"] | false;
    post.remaining = doc["remaining"] | 0;
    post.lastSeen = millis();

    return true;
  }
}
