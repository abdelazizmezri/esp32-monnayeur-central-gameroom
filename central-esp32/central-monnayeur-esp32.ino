#include <Arduino.h>
#include <WebServer.h>

#include "AppConfig.h"
#include "AppState.h"
#include "WifiService.h"
#include "WifiProvisioning.h"
#include "CoinService.h"
#include "PostService.h"
#include "StorageService.h"
#include "AuthService.h"
#include "WebRoutes.h"
#include "LogService.h"

AppState appState;
WebServer server(AppConfig::SERVER_PORT);

bool provisioningMode = false;

void setup() {
  Serial.begin(115200);
  delay(200);

  StorageService::begin();
  StorageService::load(appState);
  AuthService::begin(appState);

  String localIp;
  bool wifiOk = WifiService::begin(localIp);

  if (!wifiOk) {
    provisioningMode = true;
    WifiProvisioning::beginAccessPoint(server);
    server.begin();
    Serial.println("Provisioning server started.");
    return;
  }

  LogService::info(appState, "Mode normal démarré. IP: " + localIp);
  WifiService::beginMdns(appState);

  CoinService::begin(appState);
  PostService::begin(appState);
  WebRoutes::registerRoutes(server, appState);

  server.begin();
  LogService::info(appState, "Serveur principal démarré.");
}

void loop() {
  if (provisioningMode) {
    WifiProvisioning::handleClient();
    return;
  }

  server.handleClient();
  CoinService::update(appState);
  PostService::refreshStatuses(appState);
}
