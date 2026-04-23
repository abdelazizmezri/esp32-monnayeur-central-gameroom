#include "PosteWebServer.h"
#include "RelayService.h"

#include <ArduinoJson.h>
#include <WiFi.h>

static WebServer* gServer = nullptr;
static PosteState* gState = nullptr;

namespace PosteWebServer {

  static void sendJsonError(int code, const String& message) {
    StaticJsonDocument<128> doc;
    doc["error"] = message;

    String json;
    serializeJson(doc, json);
    gServer->send(code, "application/json", json);
  }

  static void handleStatus() {
    StaticJsonDocument<256> doc;
    doc["id"] = gState->id;
    doc["name"] = gState->name;
    doc["ip"] = WiFi.localIP().toString();
    doc["status"] = gState->status;
    doc["relay"] = gState->relayState;
    doc["remaining"] = RelayService::getRemainingSeconds(*gState);

    String json;
    serializeJson(doc, json);
    gServer->send(200, "application/json", json);
  }

  static void handleCommand() {
    if (!gServer->hasArg("plain")) {
      sendJsonError(400, "missing body");
      return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, gServer->arg("plain"));
    if (err) {
      sendJsonError(400, "invalid json");
      return;
    }

    String action = doc["action"] | "";
    unsigned long duration = doc["duration"] | 0;

    if (action == "START_SESSION") {
      if (duration == 0) {
        sendJsonError(400, "duration required");
        return;
      }
      RelayService::startSession(*gState, duration);
    } else if (action == "ADD_TIME") {
      if (duration == 0) {
        sendJsonError(400, "duration required");
        return;
      }
      RelayService::addTime(*gState, duration);
    } else if (action == "STOP_SESSION") {
      RelayService::stopSession(*gState);
    } else if (action == "PING") {
      // no-op
    } else {
      sendJsonError(400, "unknown action");
      return;
    }

    gServer->send(200, "application/json", "{\"ok\":true}");
  }

  static void handleRoot() {
    if (gState->wifiConfigPortalActive) {
      gServer->sendHeader("Location", "/wifi", true);
      gServer->send(302, "text/plain", "");
      return;
    }

    gServer->send(200, "text/plain", "ESP32 Poste OK");
  }

  void begin(WebServer& server, PosteState& state) {
    gServer = &server;
    gState = &state;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/command", HTTP_POST, handleCommand);
  }
}
