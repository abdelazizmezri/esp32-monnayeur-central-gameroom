#include "WiFiConfigService.h"
#include "PosteConfig.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WiFi.h>

static WebServer* gServer = nullptr;
static PosteState* gState = nullptr;
static DNSServer dnsServer;
static Preferences preferences;
static bool dnsStarted = false;
static bool pendingPortalShutdown = false;
static unsigned long portalShutdownAtMs = 0;

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
           parseIpv4(subnet, "Masque de sous-reseau", config.subnet, error) &&
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

  static bool connectToWifi(const String& ssid, const String& password, bool keepPortalActive,
                            const NetworkConfig& networkConfig) {
    if (ssid.isEmpty()) return false;

    Serial.print("Connecting to WiFi ");
    Serial.print(ssid);

    WiFi.mode(keepPortalActive ? WIFI_AP_STA : WIFI_STA);
    WiFi.disconnect(false, false);
    delay(100);

    if (!applyNetworkConfig(networkConfig)) {
      Serial.println("Unable to apply IP configuration.");
      return false;
    }

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startedAt < PosteConfig::WIFI_CONNECT_TIMEOUT_MS) {
      delay(250);
      Serial.print(".");
    }

    Serial.println();
    updateConnectionState(*gState);

    if (gState->wifiConnected) {
      Serial.print("Connected. IP: ");
      Serial.println(gState->wifiIp);
      return true;
    }

    Serial.println("WiFi connection failed.");
    WiFi.disconnect(!keepPortalActive, false);
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
    String apSsid = buildApSsid();
    String apPassword = PosteConfig::WIFI_SETUP_AP_PASSWORD;

    WiFi.mode(WIFI_AP_STA);
    if (apPassword.length() >= 8) {
      WiFi.softAP(apSsid.c_str(), apPassword.c_str());
    } else {
      WiFi.softAP(apSsid.c_str());
    }

    IPAddress apIp = WiFi.softAPIP();
    dnsServer.start(DNS_PORT, "*", apIp);
    dnsStarted = true;
    pendingPortalShutdown = false;
    gState->wifiConfigPortalActive = true;
    gState->wifiConnected = false;
    gState->wifiIp = "";

    Serial.print("WiFi config portal active. AP SSID: ");
    Serial.print(apSsid);
    Serial.print(" IP: ");
    Serial.println(apIp);
  }

  static void stopPortalSoon() {
    pendingPortalShutdown = true;
    portalShutdownAtMs = millis() + PosteConfig::WIFI_SETUP_AP_SHUTDOWN_DELAY_MS;
  }

  static void stopPortalNow() {
    if (dnsStarted) {
      dnsServer.stop();
      dnsStarted = false;
    }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    gState->wifiConfigPortalActive = false;
    pendingPortalShutdown = false;

    Serial.println("WiFi config portal stopped.");
  }

  static String buildNetworkOptions() {
    String options;
    int count = WiFi.scanNetworks();

    if (count <= 0) {
      options += "<option value=\"\">Aucun reseau detecte</option>";
      return options;
    }

    for (int i = 0; i < count; i++) {
      String ssid = WiFi.SSID(i);
      String label = ssid + " (" + String(WiFi.RSSI(i)) + " dBm";
      label += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? ", ouvert)" : ", securise)";
      options += "<option value=\"" + htmlEscape(ssid) + "\">" + htmlEscape(label) + "</option>";
    }

    WiFi.scanDelete();
    return options;
  }

  static void sendConfigPage(const String& message = "", bool success = false) {
    String page;
    page.reserve(8192);
    page += "<!doctype html><html lang=\"fr\"><head><meta charset=\"utf-8\">";
    page += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    page += "<title>Configuration WiFi ESP32</title>";
    page += "<style>";
    page += "body{margin:0;font-family:Arial,sans-serif;background:#f5f7fb;color:#172033}";
    page += "main{max-width:520px;margin:0 auto;padding:28px 18px}";
    page += "section{background:#fff;border:1px solid #d8deea;border-radius:8px;padding:20px;box-shadow:0 8px 24px rgba(23,32,51,.08)}";
    page += "h1{font-size:24px;margin:0 0 8px}p{line-height:1.45}label{display:block;font-weight:700;margin:16px 0 6px}";
    page += "select,input,button{box-sizing:border-box;width:100%;font-size:16px;border-radius:6px}";
    page += "select,input{border:1px solid #bac4d6;padding:12px;background:#fff}";
    page += "button{border:0;background:#1769e0;color:#fff;padding:13px 14px;margin-top:18px;font-weight:700}";
    page += ".msg{padding:12px;border-radius:6px;margin:14px 0;background:#fff4cc;color:#594400}";
    page += ".ok{background:#dff5e7;color:#145c2d}.ip{font-size:20px;font-weight:700}";
    page += "a{color:#1769e0;text-decoration:none}.small{color:#5d687b;font-size:14px}";
    page += ".network-fields{margin-top:12px;padding:2px 14px 14px;background:#f5f7fb;border-radius:6px}";
    page += "[hidden]{display:none!important}";
    page += "</style></head><body><main><section>";
    page += "<h1>Configuration WiFi</h1>";
    page += "<p class=\"small\">Connectez ce poste au reseau local utilise par le central.</p>";

    if (!message.isEmpty()) {
      page += "<div class=\"msg";
      if (success) page += " ok";
      page += "\">" + message + "</div>";
    }

    if (success && gState->wifiConnected) {
      page += "<p>Nouvelle adresse IP :</p><p class=\"ip\">" + htmlEscape(gState->wifiIp) + "</p>";
      page += "<p class=\"small\">Le point d'acces de configuration va disparaitre dans quelques secondes.</p>";
    } else {
      page += "<form method=\"post\" action=\"/wifi/save\">";
      page += "<label for=\"ssidList\">Reseau WiFi detecte</label>";
      page += "<select id=\"ssidList\" name=\"ssid\" onchange=\"document.getElementById('ssidManual').value=this.value\">" + buildNetworkOptions() + "</select>";
      page += "<label for=\"ssidManual\">Ou SSID manuel</label>";
      page += "<input id=\"ssidManual\" name=\"ssidManual\" placeholder=\"Nom du reseau\">";
      page += "<label for=\"password\">Mot de passe</label>";
      page += "<input id=\"password\" name=\"password\" type=\"password\" autocomplete=\"current-password\">";
      page += "<label for=\"networkMode\">Configuration IP</label>";
      page += "<select id=\"networkMode\" name=\"networkMode\" onchange=\"toggleNetworkFields()\">";
      page += "<option value=\"dhcp\">Automatique (DHCP)</option>";
      page += "<option value=\"static\">Manuelle (IP fixe)</option></select>";
      page += "<div id=\"manualNetworkFields\" class=\"network-fields\" hidden>";
      page += "<label for=\"localIp\">Adresse IP fixe</label>";
      page += "<input id=\"localIp\" name=\"localIp\" inputmode=\"decimal\" placeholder=\"192.168.1.51\">";
      page += "<label for=\"gateway\">Passerelle</label>";
      page += "<input id=\"gateway\" name=\"gateway\" inputmode=\"decimal\" placeholder=\"192.168.1.1\">";
      page += "<label for=\"subnet\">Masque de sous-reseau</label>";
      page += "<input id=\"subnet\" name=\"subnet\" inputmode=\"decimal\" value=\"255.255.255.0\">";
      page += "<label for=\"dns1\">DNS principal (facultatif)</label>";
      page += "<input id=\"dns1\" name=\"dns1\" inputmode=\"decimal\" placeholder=\"192.168.1.1\">";
      page += "<label for=\"dns2\">DNS secondaire (facultatif)</label>";
      page += "<input id=\"dns2\" name=\"dns2\" inputmode=\"decimal\" placeholder=\"1.1.1.1\">";
      page += "<p class=\"small\">Un proxy HTTP systeme n'est pas pris en charge. Les echanges avec la centrale restent directs sur le reseau local.</p>";
      page += "</div>";
      page += "<button type=\"submit\">Connecter</button>";
      page += "</form>";
      page += R"rawliteral(<script>
        function toggleNetworkFields() {
          var manual = document.getElementById('networkMode').value === 'static';
          document.getElementById('manualNetworkFields').hidden = !manual;
          ['localIp', 'gateway', 'subnet'].forEach(function(id) {
            document.getElementById(id).required = manual;
          });
        }
        document.getElementById('ssidManual').value = document.getElementById('ssidList').value;
        toggleNetworkFields();
      </script>)rawliteral";
      page += "<p class=\"small\"><a href=\"/wifi\">Actualiser la liste</a></p>";
    }

    page += "</section></main></body></html>";
    gServer->send(200, "text/html", page);
  }

  static void redirectToConfigPage() {
    gServer->sendHeader("Location", "/wifi", true);
    gServer->send(302, "text/plain", "");
  }

  static void handleWifiPage() {
    if (!gState->wifiConfigPortalActive) {
      gServer->send(404, "text/plain", "Not found");
      return;
    }

    sendConfigPage();
  }

  static void handleSave() {
    if (!gState->wifiConfigPortalActive) {
      gServer->send(404, "text/plain", "Not found");
      return;
    }

    String ssid = gServer->arg("ssidManual");
    ssid.trim();
    if (ssid.isEmpty()) {
      ssid = gServer->arg("ssid");
      ssid.trim();
    }

    String password = gServer->arg("password");

    if (ssid.isEmpty()) {
      sendConfigPage("SSID manquant.", false);
      return;
    }

    NetworkConfig networkConfig;
    String configError;
    if (!readNetworkConfigFromRequest(networkConfig, configError)) {
      sendConfigPage(configError, false);
      return;
    }

    bool connected = connectToWifi(ssid, password, true, networkConfig);

    if (!connected) {
      WiFi.mode(WIFI_AP_STA);
      sendConfigPage("Connexion impossible. Verifiez le mot de passe puis reessayez.", false);
      return;
    }

    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    saveNetworkConfig(networkConfig);
    preferences.end();

    stopPortalSoon();
    sendConfigPage("Connexion reussie.", true);
  }

  static void handleNotFound() {
    if (gState->wifiConfigPortalActive) {
      redirectToConfigPage();
      return;
    }

    gServer->send(404, "text/plain", "Not found");
  }

  void begin(WebServer& server, PosteState& state) {
    gServer = &server;
    gState = &state;

    server.on("/wifi", HTTP_GET, handleWifiPage);
    server.on("/wifi/save", HTTP_POST, handleSave);
    server.on("/generate_204", HTTP_GET, redirectToConfigPage);
    server.on("/gen_204", HTTP_GET, redirectToConfigPage);
    server.on("/fwlink", HTTP_GET, redirectToConfigPage);
    server.on("/hotspot-detect.html", HTTP_GET, redirectToConfigPage);
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

    if (pendingPortalShutdown && millis() >= portalShutdownAtMs) {
      stopPortalNow();
    }
  }

  bool isConnected(const PosteState& state) {
    return state.wifiConnected;
  }
}
