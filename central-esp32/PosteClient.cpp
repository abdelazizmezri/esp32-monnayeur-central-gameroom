#include "PosteClient.h"
#include "AppConfig.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace PosteClient {
  bool sendCommand(const String& ip, const String& action, long durationSeconds) {
    HTTPClient http;
    String url = "http://" + ip + "/command";

    http.begin(url);
    http.setTimeout(AppConfig::POSTE_STATUS_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");

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
    post.status = doc["status"] | "unknown";
    post.relay = doc["relay"] | false;
    post.remaining = doc["remaining"] | 0;
    post.lastSeen = millis();

    return true;
  }
}
