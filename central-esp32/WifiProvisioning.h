#pragma once

#include <Arduino.h>
#include <WebServer.h>

namespace WifiProvisioning {
  void beginAccessPoint(WebServer& server);
  void handleClient();

  bool hasSavedCredentials();
  bool connectWithSavedCredentials(String& localIp);
  void clearCredentials();
}
