#include "WifiService.h"
#include "AppConfig.h"
#include "WifiProvisioning.h"
#include "LogService.h"

#include <ESPmDNS.h>

namespace WifiService {
  bool begin(String& localIp) {
    if (!WifiProvisioning::hasSavedCredentials()) {
      return false;
    }

    return WifiProvisioning::connectWithSavedCredentials(localIp);
  }

  bool beginMdns(AppState& state) {
    if (!MDNS.begin(AppConfig::MDNS_HOSTNAME)) {
      LogService::warn(state, "mDNS non démarré.");
      return false;
    }

    MDNS.addService("http", "tcp", AppConfig::SERVER_PORT);
    LogService::info(state, String("Accès web: http://") + AppConfig::MDNS_HOSTNAME + ".local");
    return true;
  }
}
