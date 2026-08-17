#include "WiFiConfigService.h"
#include "PosteConfig.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WiFi.h>

static WebServer* gServer = nullptr;
static PosteState* gState = nullptr;
static DNSServer dnsServer;
static Preferences preferences;
static bool dnsStarted = false;

namespace WiFiConfigService {

  static const byte DNS_PORT = 53;

  struct NetworkConfig {
    bool useStaticIp = false;
    IPAddress localIp;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
  };

  static void sendNoCacheHeaders() {
    gServer->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    gServer->sendHeader("Pragma", "no-cache");
    gServer->sendHeader("Expires", "0");
  }

  static String htmlEscape(const String& value) {
    String escaped = value;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&#39;");
    return escaped;
  }

  static bool parseIpv4(const String& value, const String& fieldName, IPAddress& result,
                        String& error, bool required = true) {
    String trimmed = value;
    trimmed.trim();

    if (trimmed.isEmpty() && !required) {
      result = INADDR_NONE;
      return true;
    }

    if (trimmed.isEmpty() || !result.fromString(trimmed) || result == INADDR_NONE) {
      error = fieldName + " invalide.";
      return false;
    }

    return true;
  }

  static bool parseNetworkConfig(bool useStaticIp,
                                 const String& localIp,
                                 const String& gateway,
                                 const String& subnet,
                                 const String& dns1,
                                 const String& dns2,
                                 NetworkConfig& config,
                                 String& error) {
    config = NetworkConfig();
    config.useStaticIp = useStaticIp;

    if (!useStaticIp) {
      return true;
    }

    return parseIpv4(localIp, "Adresse IP", config.localIp, error) &&
           parseIpv4(gateway, "Passerelle", config.gateway, error) &&
           parseIpv4(subnet, "Masque de sous-réseau", config.subnet, error) &&
           parseIpv4(dns1, "DNS principal", config.dns1, error, false) &&
           parseIpv4(dns2, "DNS secondaire", config.dns2, error, false);
  }

  static bool readNetworkConfigFromRequest(NetworkConfig& config, String& error) {
    bool useStaticIp = gServer->arg("networkMode") == "static";
    return parseNetworkConfig(useStaticIp,
                              gServer->arg("localIp"),
                              gServer->arg("gateway"),
                              gServer->arg("subnet"),
                              gServer->arg("dns1"),
                              gServer->arg("dns2"),
                              config,
                              error);
  }

  static bool loadNetworkConfig(NetworkConfig& config, String& error) {
    bool useStaticIp = preferences.getBool("staticIp", false);
    return parseNetworkConfig(useStaticIp,
                              preferences.getString("localIp", ""),
                              preferences.getString("gateway", ""),
                              preferences.getString("subnet", ""),
                              preferences.getString("dns1", ""),
                              preferences.getString("dns2", ""),
                              config,
                              error);
  }

  static void saveNetworkConfig(const NetworkConfig& config) {
    preferences.putBool("staticIp", config.useStaticIp);

    if (config.useStaticIp) {
      preferences.putString("localIp", config.localIp.toString());
      preferences.putString("gateway", config.gateway.toString());
      preferences.putString("subnet", config.subnet.toString());
      preferences.putString("dns1", config.dns1 == INADDR_NONE ? "" : config.dns1.toString());
      preferences.putString("dns2", config.dns2 == INADDR_NONE ? "" : config.dns2.toString());
      return;
    }

    preferences.remove("localIp");
    preferences.remove("gateway");
    preferences.remove("subnet");
    preferences.remove("dns1");
    preferences.remove("dns2");
  }

  static bool applyNetworkConfig(const NetworkConfig& config) {
    if (!config.useStaticIp) {
      return WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }

    return WiFi.config(config.localIp, config.gateway, config.subnet, config.dns1, config.dns2);
  }

  static bool hasUsableDefaultCredentials() {
    return String(PosteConfig::WIFI_SSID) != "YOUR_WIFI" &&
           String(PosteConfig::WIFI_SSID).length() > 0;
  }

  static void updateConnectionState(PosteState& state) {
    state.wifiConnected = WiFi.status() == WL_CONNECTED;
    state.wifiIp = state.wifiConnected ? WiFi.localIP().toString() : "";
    if (state.wifiConnected) {
      state.wifiSsid = WiFi.SSID();
    }
  }

  static String buildApSsid();

  static bool connectToWifi(const String& ssid, const String& password, bool keepPortalActive,
                            const NetworkConfig& networkConfig) {
    if (ssid.isEmpty()) return false;

    WiFi.mode(keepPortalActive ? WIFI_AP_STA : WIFI_STA);

    String hostname = "poste-" + gState->chipId;
    hostname.toLowerCase();
    WiFi.setHostname(hostname.c_str());

    if (keepPortalActive && WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
      String apSsid = buildApSsid();
      WiFi.softAP(apSsid.c_str(), PosteConfig::WIFI_SETUP_AP_PASSWORD);
      delay(500);
    }

    WiFi.disconnect(false, false);
    delay(100);

    if (!applyNetworkConfig(networkConfig)) {
      Serial.println("Unable to apply IP configuration.");
      return false;
    }

    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.println();
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);

    unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startedAt < PosteConfig::WIFI_CONNECT_TIMEOUT_MS) {
      delay(500);
      Serial.print(".");
    }

    Serial.println();
    updateConnectionState(*gState);

    if (gState->wifiConnected) {
      Serial.print("Wi-Fi connected. IP: ");
      Serial.println(gState->wifiIp);
      return true;
    }

    Serial.println("Wi-Fi connection failed.");
    WiFi.disconnect(!keepPortalActive, false);
    delay(300);
    return false;
  }

  static bool connectFromStoredCredentials() {
    preferences.begin("wifi", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    NetworkConfig networkConfig;
    String configError;
    bool configOk = loadNetworkConfig(networkConfig, configError);
    preferences.end();

    if (ssid.isEmpty() && hasUsableDefaultCredentials()) {
      ssid = PosteConfig::WIFI_SSID;
      password = PosteConfig::WIFI_PASSWORD;
      networkConfig = NetworkConfig();
      configOk = true;
    }

    if (!configOk) {
      Serial.println("Invalid saved IP configuration: " + configError);
      return false;
    }

    return connectToWifi(ssid, password, false, networkConfig);
  }

  static String buildApSsid() {
    String chipId = gState->chipId;

    if (chipId.isEmpty()) {
      uint64_t mac = ESP.getEfuseMac();
      char buffer[13];
      snprintf(buffer, sizeof(buffer), "%04X%08X",
               (uint16_t)(mac >> 32),
               (uint32_t)mac);
      chipId = buffer;
    }

    chipId.toUpperCase();
    return String(PosteConfig::WIFI_SETUP_AP_SSID_PREFIX) + "-" + chipId;
  }

  static void startPortal() {
    WiFi.disconnect(true, true);
    delay(500);

    String apSsid = buildApSsid();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid.c_str(), PosteConfig::WIFI_SETUP_AP_PASSWORD);
    delay(500);

    IPAddress apIp = WiFi.softAPIP();
    dnsServer.start(DNS_PORT, "*", apIp);
    dnsStarted = true;
    gState->wifiConfigPortalActive = true;
    gState->wifiConnected = false;
    gState->wifiIp = "";

    Serial.println();
    Serial.println("=== WIFI PROVISIONING MODE ===");
    Serial.print("AP SSID: ");
    Serial.println(apSsid);
    Serial.print("AP IP: ");
    Serial.println(apIp);
    Serial.println("Open browser. If portal does not open automatically, go to http://192.168.4.1");
  }

  static String buildWifiPage(const String& message = "", bool success = false,
                              const String& ip = "") {
    int networkCount = success ? 0 : WiFi.scanNetworks();

    String optionsHtml;
    for (int i = 0; i < networkCount; i++) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);

      optionsHtml += "<option value=\"" + htmlEscape(ssid) + "\">" +
                     htmlEscape(ssid) + " (" + String(rssi) + " dBm)</option>";
    }

    String infoBlock = "";
    if (message.length() > 0) {
      infoBlock += "<div class='msg";
      if (success) infoBlock += " ok";
      infoBlock += "'>" + htmlEscape(message) + "</div>";
    }

    if (success && ip.length() > 0) {
      String hostname = "poste-" + gState->chipId;
      hostname.toLowerCase();
      infoBlock += "<p>Nouvelle adresse IP :</p><p class='ip'>" + htmlEscape(ip) + "</p>";
      infoBlock += "<p>Nom de domaine :</p><p class='ip'>http://" + htmlEscape(hostname) + ".local</p>";
      infoBlock += "<p class='small'>L'ESP32 va redémarrer automatiquement dans quelques secondes. "
                   "Reconnecte ton PC au réseau local puis ouvre l'adresse affichée.</p>";
    }

    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Configuration Wi-Fi</title>
  <style>
    body{margin:0;font-family:Arial,sans-serif;background:#f5f7fb;color:#172033}
    main{max-width:520px;margin:0 auto;padding:28px 18px}
    section{background:#fff;border:1px solid #d8deea;border-radius:8px;padding:20px;box-shadow:0 8px 24px rgba(23,32,51,.08)}
    h1{font-size:24px;margin:0 0 8px}p{line-height:1.45}label{display:block;font-weight:700;margin:16px 0 6px}
    select,input,button{box-sizing:border-box;width:100%;font-size:16px;border-radius:6px}
    select,input{border:1px solid #bac4d6;padding:12px;background:#fff}
    button{border:0;background:#1769e0;color:#fff;padding:13px 14px;margin-top:18px;font-weight:700;cursor:pointer}
    .msg{padding:12px;border-radius:6px;margin:14px 0;background:#fff4cc;color:#594400}
    .ok{background:#dff5e7;color:#145c2d}.ip{font-size:20px;font-weight:700;margin:4px 0 12px;word-break:break-word}
    a{color:#1769e0;text-decoration:none}.small,.muted{color:#5d687b;font-size:14px}
    .network-fields{margin-top:12px;padding:2px 14px 14px;background:#f5f7fb;border-radius:6px}
    [hidden]{display:none!important}
  </style>
</head>
<body>
  <main><section>
    <h1>Configuration Wi-Fi</h1>
    <p class="small">Connecte ce poste ESP32 au réseau local utilisé par la centrale.</p>
)rawliteral";

    html += infoBlock;

    if (!success) {
    html += R"rawliteral(
    <form method="POST" action="/wifi/save">
      <label for="ssidList">Réseaux disponibles</label>
      <select id="ssidList" onchange="document.getElementById('ssid').value=this.value">
        <option value="">-- Sélectionner un réseau --</option>
)rawliteral";

    html += optionsHtml;

    html += R"rawliteral(
      </select>

      <label for="ssid">SSID</label>
      <input id="ssid" name="ssid" placeholder="Nom du Wi-Fi" required>

      <label for="password">Mot de passe</label>
      <input id="password" name="password" type="password" placeholder="Mot de passe Wi-Fi">

      <label for="networkMode">Configuration IP</label>
      <select id="networkMode" name="networkMode" onchange="toggleNetworkFields()">
        <option value="dhcp">Automatique (DHCP)</option>
        <option value="static">Manuelle (IP fixe)</option>
      </select>

      <div id="manualNetworkFields" class="network-fields" hidden>
        <label for="localIp">Adresse IP fixe</label>
        <input id="localIp" name="localIp" inputmode="decimal" placeholder="192.168.1.50">

        <label for="gateway">Passerelle</label>
        <input id="gateway" name="gateway" inputmode="decimal" placeholder="192.168.1.1">

        <label for="subnet">Masque de sous-réseau</label>
        <input id="subnet" name="subnet" inputmode="decimal" value="255.255.255.0">

        <label for="dns1">DNS principal (facultatif)</label>
        <input id="dns1" name="dns1" inputmode="decimal" placeholder="192.168.1.1">

        <label for="dns2">DNS secondaire (facultatif)</label>
        <input id="dns2" name="dns2" inputmode="decimal" placeholder="1.1.1.1">

        <p class="muted">Un proxy HTTP système n'est pas pris en charge. Les échanges entre la centrale et les postes restent directs sur le réseau local.</p>
      </div>

      <button type="submit">Enregistrer et connecter</button>
    </form>

    <script>
      function toggleNetworkFields() {
        var manual = document.getElementById('networkMode').value === 'static';
        document.getElementById('manualNetworkFields').hidden = !manual;
        ['localIp', 'gateway', 'subnet'].forEach(function(id) {
          document.getElementById(id).required = manual;
        });
      }
      toggleNetworkFields();
    </script>

    <p class="muted">
      Après succès, utilise l'adresse IP ou le nom de domaine affiché.
    </p>
)rawliteral";
    }

    html += R"rawliteral(
  </section></main>
</body>
</html>
)rawliteral";

    return html;
  }

  static void handleWifiPage() {
    if (!gState->wifiConfigPortalActive) {
      gServer->send(404, "text/plain", "Not found");
      return;
    }

    sendNoCacheHeaders();
    gServer->send(200, "text/html", buildWifiPage());
  }

  static void handleSave() {
    if (!gState->wifiConfigPortalActive) {
      gServer->send(404, "text/plain", "Not found");
      return;
    }

    String ssid = gServer->arg("ssid");
    String password = gServer->arg("password");

    if (ssid.isEmpty()) {
      sendNoCacheHeaders();
      gServer->send(400, "text/html", buildWifiPage("SSID obligatoire.", false));
      return;
    }

    NetworkConfig networkConfig;
    String configError;
    if (!readNetworkConfigFromRequest(networkConfig, configError)) {
      sendNoCacheHeaders();
      gServer->send(400, "text/html", buildWifiPage(configError, false));
      return;
    }

    bool connected = connectToWifi(ssid, password, true, networkConfig);

    if (!connected) {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage("Échec de connexion Wi-Fi. Vérifie les identifiants.", false));
      return;
    }

    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    saveNetworkConfig(networkConfig);
    preferences.end();

    sendNoCacheHeaders();
    gServer->send(200, "text/html", buildWifiPage("Connexion réussie au Wi-Fi.", true, gState->wifiIp));
    delay(8000);
    ESP.restart();
  }

  static void handleNotFound() {
    if (gState->wifiConfigPortalActive) {
      gServer->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/wifi", true);
      gServer->send(302, "text/plain", "");
      return;
    }

    gServer->send(404, "text/plain", "Not found");
  }

  void begin(WebServer& server, PosteState& state) {
    gServer = &server;
    gState = &state;

    server.on("/wifi", HTTP_GET, handleWifiPage);
    server.on("/wifi/scan", HTTP_GET, []() {
      if (!gState->wifiConfigPortalActive) {
        gServer->send(404, "text/plain", "Not found");
        return;
      }

      int count = WiFi.scanNetworks();

      StaticJsonDocument<2048> doc;
      JsonArray arr = doc.createNestedArray("networks");

      for (int i = 0; i < count; i++) {
        JsonObject item = arr.createNestedObject();
        item["ssid"] = WiFi.SSID(i);
        item["rssi"] = WiFi.RSSI(i);
        item["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
      }

      String json;
      serializeJson(doc, json);
      sendNoCacheHeaders();
      gServer->send(200, "application/json", json);
    });
    server.on("/wifi/save", HTTP_POST, handleSave);

    auto sendCaptivePortalPage = []() {
      if (!gState->wifiConfigPortalActive) {
        gServer->send(404, "text/plain", "Not found");
        return;
      }

      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    };

    server.on("/generate_204", HTTP_GET, sendCaptivePortalPage);
    server.on("/gen_204", HTTP_GET, sendCaptivePortalPage);
    server.on("/hotspot-detect.html", HTTP_GET, sendCaptivePortalPage);
    server.on("/library/test/success.html", HTTP_GET, sendCaptivePortalPage);
    server.on("/connecttest.txt", HTTP_GET, sendCaptivePortalPage);
    server.on("/ncsi.txt", HTTP_GET, sendCaptivePortalPage);
    server.on("/redirect", HTTP_GET, sendCaptivePortalPage);
    server.on("/fwlink", HTTP_GET, []() {
      if (!gState->wifiConfigPortalActive) {
        gServer->send(404, "text/plain", "Not found");
        return;
      }

      gServer->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/wifi", true);
      gServer->send(302, "text/plain", "");
    });
    server.onNotFound(handleNotFound);

    if (!connectFromStoredCredentials()) {
      startPortal();
    }
  }

  void loop(PosteState& state) {
    updateConnectionState(state);

    if (dnsStarted) {
      dnsServer.processNextRequest();
    }

    if (!state.wifiConnected && !state.wifiConfigPortalActive) {
      startPortal();
    }
  }

  bool isConnected(const PosteState& state) {
    return state.wifiConnected;
  }
}
