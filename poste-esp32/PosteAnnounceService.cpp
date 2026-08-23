#include "PosteAnnounceService.h"

#include "PosteConfig.h"
#include "RelayService.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace PosteAnnounceService {
  static bool mdnsReady = false;
  static bool centralIpCached = false;
  static IPAddress cachedCentralIp;

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
    if (centralIpCached) {
      centralIp = cachedCentralIp;
      return true;
    }

    centralIp = MDNS.queryHost(PosteConfig::CENTRAL_MDNS_HOSTNAME,
                               PosteConfig::CENTRAL_MDNS_QUERY_TIMEOUT_MS);
    if (centralIp == IPAddress(0, 0, 0, 0)) {
      return false;
    }

    cachedCentralIp = centralIp;
    centralIpCached = true;
    return true;
  }

  void update(PosteState& state) {
    static unsigned long lastAttemptAt = 0;
    static uint32_t lastAnnouncedRevision = 0;
    static bool hasAttempted = false;
    static bool lastAttemptFailed = false;
    static bool hasSuccessfulAnnounce = false;

    if (!state.wifiConnected || state.wifiConfigPortalActive) {
      return;
    }

    RelayService::SessionSnapshot session = RelayService::getSessionSnapshot();
    bool sessionChanged = !hasSuccessfulAnnounce ||
                          session.revision != lastAnnouncedRevision;
    unsigned long requiredInterval = lastAttemptFailed
                                         ? PosteConfig::ANNOUNCE_RETRY_INTERVAL_MS
                                         : (sessionChanged
                                                ? PosteConfig::ANNOUNCE_STATE_CHANGE_DELAY_MS
                                                : PosteConfig::ANNOUNCE_INTERVAL_MS);
    unsigned long now = millis();
    if (hasAttempted && now - lastAttemptAt < requiredInterval) {
      return;
    }

    hasAttempted = true;
    lastAttemptAt = now;

    if (!ensureMdns(state)) {
      lastAttemptFailed = true;
      return;
    }

    IPAddress centralIp;
    if (!resolveCentral(centralIp)) {
      lastAttemptFailed = true;
      return;
    }

    HTTPClient http;
    String url = "http://" + centralIp.toString() + "/poste/announce";
    http.begin(url);
    http.setConnectTimeout(PosteConfig::ANNOUNCE_HTTP_CONNECT_TIMEOUT_MS);
    http.setTimeout(PosteConfig::ANNOUNCE_HTTP_RESPONSE_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + PosteConfig::COMMAND_TOKEN);

    // Reprendre un instantane apres la resolution mDNS afin d'envoyer l'etat le
    // plus recent si la tache de controle a fait expirer la session entre-temps.
    session = RelayService::getSessionSnapshot();

    StaticJsonDocument<512> doc;
    doc["chipId"] = state.chipId;
    doc["configured"] = state.configured;
    doc["id"] = state.id;
    doc["name"] = state.name;
    doc["status"] = RelayService::statusToString(session.status);
    doc["relay"] = session.relayState;
    doc["remaining"] = session.remainingSeconds;
    doc["recoveryPending"] = session.recoveryPending;
    doc["recoveryRemaining"] = session.recoveryRemainingSeconds;
    doc["ip"] = WiFi.localIP().toString();

    String body;
    serializeJson(doc, body);
    int code = http.POST(body);
    http.end();

    bool accepted = code >= 200 && code < 300;
    lastAttemptFailed = !accepted;

    if (code <= 0) {
      centralIpCached = false;
      cachedCentralIp = IPAddress();
    }

    if (accepted) {
      hasSuccessfulAnnounce = true;
      lastAnnouncedRevision = session.revision;
    }
  }
}
