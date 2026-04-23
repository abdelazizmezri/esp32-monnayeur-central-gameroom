#include "WifiProvisioning.h"
#include "AppConfig.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <DNSServer.h>

static WebServer* gServer = nullptr;
static Preferences prefs;
static DNSServer dnsServer;

static const byte DNS_PORT = 53;
static bool provisioningMode = false;

static void sendNoCacheHeaders() {
  gServer->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  gServer->sendHeader("Pragma", "no-cache");
  gServer->sendHeader("Expires", "0");
}

static String htmlEscape(const String& input) {
  String out = input;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  out.replace("'", "&#39;");
  return out;
}

static void beginProvisioningNetwork() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(AppConfig::MDNS_HOSTNAME);
  WiFi.softAP(AppConfig::AP_SSID, AppConfig::AP_PASSWORD);
  delay(500);

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

static bool tryConnectToWifi(const String& ssid, const String& password, String& localIp, bool keepAccessPoint) {
  WiFi.mode(keepAccessPoint ? WIFI_AP_STA : WIFI_STA);
  WiFi.setHostname(AppConfig::MDNS_HOSTNAME);

  if (keepAccessPoint && WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
    WiFi.softAP(AppConfig::AP_SSID, AppConfig::AP_PASSWORD);
    delay(500);
  }

  WiFi.disconnect(false, false);
  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < AppConfig::WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    localIp = WiFi.localIP().toString();
    Serial.println();
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(localIp);
    return true;
  }

  Serial.println();
  Serial.println("Wi-Fi connection failed.");
  WiFi.disconnect(!keepAccessPoint, false);
  delay(300);
  return false;
}

static String buildWifiPage(const String& message = "", bool success = false, const String& ip = "") {
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
    String bg = success ? "#dcfce7" : "#fee2e2";
    String color = success ? "#166534" : "#991b1b";

    infoBlock += "<div style='margin-bottom:16px;padding:12px;border-radius:12px;background:" + bg +
                 ";color:" + color + ";font-weight:700;'>" + htmlEscape(message) + "</div>";
  }

  if (success && ip.length() > 0) {
    infoBlock += "<div style='margin-bottom:16px;padding:12px;border-radius:12px;background:#dbeafe;"
                 "color:#1d4ed8;font-weight:700;'>Nouvelle adresse IP : " + htmlEscape(ip) + "</div>";
    infoBlock += "<div style='margin-bottom:16px;padding:12px;border-radius:12px;background:#f8fafc;"
                 "color:#334155;'>L'ESP32 va redémarrer automatiquement dans quelques secondes. "
                 "Reconnecte ton PC au réseau local puis ouvre l'adresse affichée.</div>";
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Configuration Wi-Fi</title>
  <style>
    body{font-family:Arial,sans-serif;background:#0f172a;margin:0;padding:20px;color:#fff}
    .card{max-width:560px;margin:30px auto;background:#fff;color:#0f172a;border-radius:18px;padding:20px;box-shadow:0 10px 30px rgba(0,0,0,.2)}
    h1{margin-top:0}
    label{display:block;margin:10px 0 6px;font-weight:bold}
    input,select{width:100%;padding:12px;border-radius:12px;border:1px solid #ddd;box-sizing:border-box}
    button{margin-top:16px;width:100%;padding:12px;border:none;border-radius:12px;background:#2563eb;color:#fff;font-weight:bold;cursor:pointer}
    .muted{color:#64748b;font-size:13px;margin-top:8px}
    .row{display:flex;gap:10px}
    .row > div{flex:1}
  </style>
</head>
<body>
  <div class="card">
    <h1>Configuration Wi-Fi</h1>
)rawliteral";

  html += infoBlock;

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

      <button type="submit">Enregistrer et connecter</button>
    </form>

    <div class="muted">
      Connecte l'ESP32 à ton Wi-Fi local. Après succès, utilise la nouvelle IP affichée.
    </div>
  </div>
</body>
</html>
)rawliteral";

  return html;
}

namespace WifiProvisioning {

  bool hasSavedCredentials() {
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    prefs.end();
    return ssid.length() > 0;
  }

  bool connectWithSavedCredentials(String& localIp) {
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");
    prefs.end();

    if (ssid.isEmpty()) {
      return false;
    }

    return tryConnectToWifi(ssid, password, localIp, false);
  }

  bool saveAndConnect(const String& ssid, const String& password, String& localIp) {
    if (!tryConnectToWifi(ssid, password, localIp, provisioningMode)) {
      return false;
    }

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
    return true;
  }

  void clearCredentials() {
    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();
  }

  void beginAccessPoint(WebServer& server) {
    gServer = &server;
    provisioningMode = true;

    WiFi.disconnect(true, true);
    delay(500);

    beginProvisioningNetwork();

    IPAddress apIp = WiFi.softAPIP();

    Serial.println();
    Serial.println("=== WIFI PROVISIONING MODE ===");
    Serial.print("AP SSID: ");
    Serial.println(AppConfig::AP_SSID);
    Serial.print("AP IP: ");
    Serial.println(apIp);
    Serial.println("Open browser. If portal does not open automatically, go to http://192.168.4.1");

    gServer->on("/", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/wifi", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/wifi/scan", HTTP_GET, []() {
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

    gServer->on("/wifi/save", HTTP_POST, []() {
      String ssid = gServer->arg("ssid");
      String password = gServer->arg("password");

      if (ssid.isEmpty()) {
        sendNoCacheHeaders();
        gServer->send(400, "text/html", buildWifiPage("SSID obligatoire.", false));
        return;
      }

      String localIp;
      bool ok = saveAndConnect(ssid, password, localIp);

      if (!ok) {
        sendNoCacheHeaders();
        gServer->send(200, "text/html", buildWifiPage("Échec de connexion Wi-Fi. Vérifie les identifiants.", false));
        return;
      }

      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage("Connexion réussie au Wi-Fi.", true, localIp));
      delay(8000);
      ESP.restart();
    });

    // Endpoints captifs courants pour iOS / Android / Windows
    gServer->on("/generate_204", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/gen_204", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/hotspot-detect.html", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/library/test/success.html", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/connecttest.txt", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/ncsi.txt", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->on("/fwlink", HTTP_GET, []() {
      gServer->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/wifi", true);
      gServer->send(302, "text/plain", "");
    });

    gServer->on("/redirect", HTTP_GET, []() {
      sendNoCacheHeaders();
      gServer->send(200, "text/html", buildWifiPage());
    });

    gServer->onNotFound([]() {
      gServer->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/wifi", true);
      gServer->send(302, "text/plain", "");
    });
  }

  void handleClient() {
    if (!provisioningMode || !gServer) {
      return;
    }

    dnsServer.processNextRequest();
    gServer->handleClient();
  }
}
