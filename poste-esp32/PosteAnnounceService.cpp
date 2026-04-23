#include "PosteAnnounceService.h"

#include "PosteConfig.h"
#include "RelayService.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace PosteAnnounceService {
  static bool mdnsReady = false;

  static bool ensureMdns(PosteState& state) {
    if (mdnsReady) {
      return true;
    }

    String hostname = "poste-" + state.chipId;
    hostname.toLowerCase();
    mdnsReady = MDNS.begin(hostname.c_str());
    return mdnsReady;
  }

  static bool resolveCentral(IPAddress& centralIp) {
    centralIp = MDNS.queryHost(PosteConfig::CENTRAL_MDNS_HOSTNAME);
    return centralIp != IPAddress(0, 0, 0, 0);
  }

  void update(PosteState& state) {
    static unsigned long lastAnnounceAt = 0;

    if (!state.wifiConnected || state.wifiConfigPortalActive) {
      return;
    }

    if (millis() - lastAnnounceAt < PosteConfig::ANNOUNCE_INTERVAL_MS) {
      return;
    }

    lastAnnounceAt = millis();

    if (!ensureMdns(state)) {
      return;
    }

    IPAddress centralIp;
    if (!resolveCentral(centralIp)) {
      return;
    }

    HTTPClient http;
    String url = "http://" + centralIp.toString() + "/poste/announce";
    http.begin(url);
    http.setTimeout(1200);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + PosteConfig::COMMAND_TOKEN);

    StaticJsonDocument<384> doc;
    doc["chipId"] = state.chipId;
    doc["configured"] = state.configured;
    doc["id"] = state.id;
    doc["name"] = state.name;
    doc["status"] = state.status;
    doc["relay"] = state.relayState;
    doc["remaining"] = RelayService::getRemainingSeconds(state);
    doc["ip"] = WiFi.localIP().toString();

    String body;
    serializeJson(doc, body);
    http.POST(body);
    http.end();
  }
}
