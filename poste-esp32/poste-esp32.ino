#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "PosteConfig.h"
#include "PosteState.h"
#include "RelayService.h"
#include "PosteWebServer.h"
#include "WiFiConfigService.h"
#include "PosteIdentityService.h"
#include "PosteAnnounceService.h"

PosteState posteState;
WebServer server(PosteConfig::HTTP_PORT);

void setup() {
  Serial.begin(115200);
  delay(200);

  PosteIdentityService::begin(posteState);
  posteState.status = "idle";
  posteState.relayState = false;
  posteState.endTimeMs = 0;

  RelayService::begin(posteState);

  PosteWebServer::begin(server, posteState);
  WiFiConfigService::begin(server, posteState);

  server.begin();
  Serial.println("Poste HTTP server started.");
}

void loop() {
  server.handleClient();
  WiFiConfigService::loop(posteState);

  RelayService::update(posteState);
  PosteAnnounceService::update(posteState);
}
