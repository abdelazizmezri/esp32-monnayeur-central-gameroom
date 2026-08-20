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

    String networksHtml;
    for (int i = 0; i < networkCount; i++) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      networksHtml += "<button class='network' type='button' data-ssid=\"" + htmlEscape(ssid) +
                      "\" onclick='pickNetwork(this)'><span class='radio'></span><b>" +
                      htmlEscape(ssid) + "</b><small>" + String(rssi) + " dBm</small></button>";
    }
    if (!success && networksHtml.isEmpty()) {
      networksHtml = "<p class='empty' id='emptyNetworks'>Aucun réseau détecté. Lance une nouvelle recherche.</p>";
    }

    String infoBlock = "";
    if (message.length() > 0) {
      if (success) infoBlock += "<section class='success'>";
      infoBlock += "<div class='msg";
      if (success) infoBlock += " ok";
      infoBlock += "'>" + htmlEscape(message) + "</div>";
    }

    if (success && ip.length() > 0) {
      infoBlock += "<div class='result'><span>Nouvelle adresse IP</span><strong>" + htmlEscape(ip) + "</strong></div>";
      infoBlock += "<p class='muted'>Le Poste redémarrera automatiquement. Une fois connecté au réseau local, il sera détecté par la Centrale.</p>";
    }
    if (success && message.length() > 0) infoBlock += "</section>";

    String html = R"rawliteral(
<!doctype html><html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#020817"><title>Configuration Wi-Fi du Poste</title><style>
:root{color-scheme:dark;--bg:#020817;--surface:#071426;--surface2:#0a192c;--line:#27405d;--text:#f4f7ff;--muted:#9aaac4;--blue:#1681ff;--cyan:#19d8ff;--violet:#7941ff;--green:#27dda0}*{box-sizing:border-box}body{margin:0;min-height:100vh;overflow-x:hidden;color:var(--text);background:radial-gradient(circle at 82% 12%,#0a2145 0,transparent 31%),linear-gradient(145deg,#020817,#041328 55%,#020817);font-family:Inter,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}.shell{display:grid;grid-template-columns:clamp(220px,24vw,300px) minmax(0,1fr);width:100%;min-height:100vh;overflow-x:hidden}.side{position:relative;display:flex;flex-direction:column;align-items:center;padding:38px 28px;overflow:hidden;border-right:1px solid #17304c;background:linear-gradient(180deg,#030b1d,#06152d)}.side:before{content:"";position:absolute;inset:0;opacity:.22;background-image:linear-gradient(#12335c 1px,transparent 1px),linear-gradient(90deg,#12335c 1px,transparent 1px);background-size:46px 46px;mask-image:linear-gradient(#000,transparent 72%)}.brand,.device,.tag,.side h2,.steps{position:relative}.brand{text-align:center;font-size:25px;font-weight:900;letter-spacing:.08em}.brand small{display:block;margin-top:3px;color:#7ecbff;font-size:12px;letter-spacing:.2em}.device{display:grid;place-items:center;width:150px;height:150px;margin:38px 0 18px;border:1px solid #184e83;border-radius:50%;color:#65caff;background:radial-gradient(circle,#0b3567,#06162f 68%);box-shadow:0 0 45px #0965c74d}.device svg{width:100px;height:100px;filter:drop-shadow(0 0 12px #1a9cff)}.tag{padding:7px 22px;border:1px solid #8653ff;border-radius:7px;color:#d7c7ff;font-size:12px;font-weight:800;letter-spacing:.16em}.side h2{margin:22px 0 26px;text-align:center;font-size:18px}.steps{align-self:stretch;display:grid;gap:20px;margin:0;padding:0;list-style:none}.steps li{display:flex;align-items:center;gap:12px;color:#8295b3;font-size:14px}.steps span{display:grid;place-items:center;width:32px;height:32px;border:1px solid #536a8d;border-radius:50%;font-weight:800}.steps .on{color:#fff;font-weight:700}.steps .on span{border-color:#1681ff;background:#1277ed;box-shadow:0 0 20px #1681ff66}.main{min-width:0;width:100%;max-width:none;margin:0;padding:clamp(20px,2.5vw,34px)}.top{display:flex;min-width:0;flex-wrap:wrap;align-items:flex-start;justify-content:space-between;gap:20px;margin-bottom:22px}.top h1{margin:0 0 7px;font-size:clamp(27px,3vw,40px);letter-spacing:-.035em}.top p{margin:0;color:#a9b5ce}.setup{display:inline-flex;align-items:center;gap:8px;padding:9px 13px;border:1px solid #22568d;border-radius:8px;color:#69aeff;background:#071c36;font-size:12px;white-space:nowrap}.grid{display:grid;grid-template-columns:minmax(0,.92fr) minmax(0,1.08fr);border:1px solid var(--line);border-radius:14px;overflow:hidden;background:#061225;box-shadow:0 22px 60px #0005}.panel{min-width:0;padding:24px}.panel+.panel{border-left:1px solid var(--line)}.panel-head{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:16px}.panel h2{margin:0;font-size:17px}.refresh{width:auto;margin:0;padding:9px 12px;border:1px solid #2565ad;color:#8ec5ff;background:#081c37}.networks{display:grid;gap:10px}.network{display:grid;grid-template-columns:20px 1fr auto;align-items:center;gap:11px;width:100%;margin:0;padding:15px;border:1px solid #29415e;border-radius:9px;color:#dce8f8;text-align:left;background:#08172b}.network:hover,.network.active{border-color:#1681ff;background:linear-gradient(90deg,#0a2852,#09203d);box-shadow:inset 0 0 24px #0877ff18}.network b{min-width:0;overflow:hidden;overflow-wrap:anywhere;text-overflow:ellipsis}.network small{color:#7891ad}.radio{width:17px;height:17px;border:2px solid #7487a5;border-radius:50%}.network.active .radio{border:5px solid #2788ff}.empty{padding:18px;border:1px dashed #29415e;border-radius:9px;color:#8195b2;text-align:center}.scan{min-height:18px;margin:10px 0 0;color:#8195b2;font-size:12px}label{display:block;margin:0 0 7px;color:#c8d4e6;font-size:13px}input,button{font:inherit}input{width:100%;height:47px;margin-bottom:16px;padding:0 13px;border:1px solid #2b425d;border-radius:8px;outline:0;color:#fff;background:#071528}input:focus{border-color:#1681ff;box-shadow:0 0 0 3px #1681ff1e}.password{position:relative}.password input{padding-right:48px}.eye{position:absolute;right:3px;top:3px;width:42px;height:41px;margin:0;border:0;color:#8ea3bf;background:transparent}.mode-label{margin-top:1px}.modes{display:grid;grid-template-columns:1fr 1fr;margin-bottom:15px;border:1px solid #2b425d;border-radius:8px;overflow:hidden}.modes label{margin:0}.modes input{position:absolute;opacity:0;pointer-events:none}.modes span{display:grid;place-items:center;min-height:45px;padding:8px;color:#9caeca;text-align:center;cursor:pointer}.modes input:checked+span{color:#fff;background:linear-gradient(135deg,#126fe7,#4633e7)}details{margin:0 0 16px;border:1px solid #2b425d;border-radius:8px;background:#061326}summary{padding:13px;cursor:pointer;color:#b5c4d9}.fields{padding:0 13px 2px}.submit{width:100%;min-height:50px;margin:0;border:0;border-radius:8px;color:#fff;background:linear-gradient(110deg,#0d82ff,#384cf4 55%,#8135ef);font-weight:800;cursor:pointer;box-shadow:0 12px 28px #2d4ff52c}button{border-radius:8px;cursor:pointer}.submit:hover,.refresh:hover{filter:brightness(1.13)}button:disabled{opacity:.6;cursor:wait}.hint,.success{margin-top:18px;padding:16px 18px;border:1px solid #167fa5;border-radius:10px;color:#67dcff;background:#062039}.msg{margin-bottom:16px;padding:13px;border:1px solid #a56e22;border-radius:9px;color:#ffd184;background:#39270e}.msg.ok{border-color:#208e6c;color:#7ff0c5;background:#092d27}.success{max-width:760px;margin:35px auto;text-align:center}.result{display:grid;gap:8px;margin:18px 0;padding:17px;border:1px solid #285078;border-radius:10px;background:#06152b}.result span{color:#91a4bf;font-size:12px}.result strong{word-break:break-word;font-size:19px}.muted{color:var(--muted);line-height:1.5}[hidden]{display:none!important}@media(max-width:1100px) and (min-width:761px){.shell{grid-template-columns:220px minmax(0,1fr)}.side{padding:24px 16px}.device{width:112px;height:112px;margin:22px 0 14px}.device svg{width:72px;height:72px}.side h2{margin:16px 0 20px;font-size:16px}.main{padding:22px}.panel{padding:18px}.top h1{font-size:30px}.setup{font-size:11px}}@media(max-width:760px){.shell{display:block}.side{min-height:auto;padding:22px}.device{display:none}.side h2{margin:14px 0}.steps{grid-template-columns:repeat(3,1fr);gap:8px}.steps li{gap:7px;font-size:11px}.steps span{width:27px;height:27px}.main{padding:24px 15px}.grid{grid-template-columns:1fr}.panel+.panel{border-left:0;border-top:1px solid var(--line)}}@media(max-width:520px){.top{display:block}.setup{margin-top:14px}.panel{padding:18px}.steps li{display:block;text-align:center}.steps span{margin:0 auto 5px}.network{grid-template-columns:20px 1fr}.network small{display:none}}
</style></head><body><div class="shell"><aside class="side"><div class="brand">PS TIME<small>MANAGER</small></div><div class="device"><svg viewBox="0 0 120 120" fill="none" stroke="currentColor" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"><path d="M31 44c-8 2-13 18-16 31-3 14 8 22 18 11l12-13h30l12 13c10 11 21 3 18-11-3-13-8-29-16-31-12-3-46-3-58 0z"/><path d="M34 57v15m-8-7h16m36-5h.1m12 9h.1"/><circle cx="60" cy="60" r="8"/></svg></div><div class="tag">POSTE</div><h2>Connecter ce Poste</h2><ol class="steps"><li class="on"><span>1</span>Réseau Wi-Fi</li><li><span>2</span>Configuration IP</li><li><span>3</span>Connexion à la Centrale</li></ol></aside><main class="main"><header class="top"><div><h1>Configuration Wi-Fi du Poste</h1><p>Sélectionnez le réseau local utilisé dans votre salle de jeu.</p></div><span class="setup">⚙ Mode configuration</span></header>
)rawliteral";

    html += infoBlock;

    if (!success) {
    html += R"rawliteral(
<div class="grid"><section class="panel"><div class="panel-head"><h2>Réseaux détectés</h2><button id="refreshNetworks" class="refresh" type="button" onclick="refreshWifiList()">↻ Actualiser</button></div><div id="networkList" class="networks">
)rawliteral";

    html += networksHtml;

    html += R"rawliteral(
</div><p id="scanStatus" class="scan" aria-live="polite"></p></section><form class="panel" method="POST" action="/wifi/save"><label for="ssid">SSID</label><input id="ssid" name="ssid" placeholder="Nom du réseau Wi-Fi" required><label for="password">Mot de passe</label><div class="password"><input id="password" name="password" type="password" placeholder="Mot de passe Wi-Fi"><button class="eye" type="button" onclick="togglePassword()" aria-label="Afficher ou masquer le mot de passe">◉</button></div><label class="mode-label">Configuration IP</label><div class="modes"><label><input type="radio" name="networkMode" value="dhcp" checked onchange="toggleNetworkFields()"><span>Automatique (DHCP)</span></label><label><input type="radio" name="networkMode" value="static" onchange="toggleNetworkFields()"><span>Manuelle (IP fixe)</span></label></div><details id="manualNetworkFields" hidden><summary>Paramètres IP avancés</summary><div class="fields"><label for="localIp">Adresse IP fixe</label><input id="localIp" name="localIp" inputmode="decimal" placeholder="192.168.1.50"><label for="gateway">Passerelle</label><input id="gateway" name="gateway" inputmode="decimal" placeholder="192.168.1.1"><label for="subnet">Masque de sous-réseau</label><input id="subnet" name="subnet" inputmode="decimal" value="255.255.255.0"><label for="dns1">DNS principal (facultatif)</label><input id="dns1" name="dns1" inputmode="decimal" placeholder="192.168.1.1"><label for="dns2">DNS secondaire (facultatif)</label><input id="dns2" name="dns2" inputmode="decimal" placeholder="1.1.1.1"></div></details><button class="submit" type="submit">Enregistrer et connecter</button></form></div><div class="hint">ⓘ Après connexion, le Poste sera automatiquement détecté par la Centrale.</div><script>
function pickNetwork(b){document.querySelectorAll('.network').forEach(function(x){x.classList.remove('active')});b.classList.add('active');document.getElementById('ssid').value=b.dataset.ssid||''}
function addNetwork(n){var b=document.createElement('button');b.type='button';b.className='network';b.dataset.ssid=n.ssid||'';b.onclick=function(){pickNetwork(b)};var r=document.createElement('span');r.className='radio';var name=document.createElement('b');name.textContent=b.dataset.ssid;var q=document.createElement('small');q.textContent=String(n.rssi)+' dBm';b.append(r,name,q);document.getElementById('networkList').appendChild(b)}
async function refreshWifiList(){var b=document.getElementById('refreshNetworks'),l=document.getElementById('networkList'),s=document.getElementById('scanStatus');b.disabled=true;b.textContent='Recherche…';s.textContent='';try{var r=await fetch('/wifi/scan',{cache:'no-store'});if(!r.ok)throw 0;var d=await r.json(),n=Array.isArray(d.networks)?d.networks:[];l.replaceChildren();n.forEach(addNetwork);if(!n.length){var e=document.createElement('p');e.className='empty';e.textContent='Aucun réseau détecté.';l.appendChild(e)}s.textContent=n.length?n.length+' réseau(x) détecté(s).':'Aucun réseau détecté.'}catch(e){s.textContent='Impossible d’actualiser la liste.'}finally{b.disabled=false;b.textContent='↻ Actualiser'}}
function toggleNetworkFields(){var m=document.querySelector('input[name="networkMode"]:checked').value==='static',d=document.getElementById('manualNetworkFields');d.hidden=!m;if(m)d.open=true;['localIp','gateway','subnet'].forEach(function(id){document.getElementById(id).required=m})}function togglePassword(){var p=document.getElementById('password');p.type=p.type==='password'?'text':'password'}toggleNetworkFields();</script>
)rawliteral";
    }

    html += R"rawliteral(
</main></div></body></html>
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
