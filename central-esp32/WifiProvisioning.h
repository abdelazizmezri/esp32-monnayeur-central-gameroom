#pragma once

#include <Arduino.h>
#include <WebServer.h>

namespace WifiProvisioning {
  void beginAccessPoint(WebServer& server);
  void handleClient();

  bool hasSavedCredentials();
  bool connectWithSavedCredentials(String& localIp);
  bool saveAndConnect(const String& ssid, const String& password, String& localIp);
  void clearCredentials();
}