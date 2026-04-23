#pragma once

#include <Arduino.h>

#include "AppState.h"

namespace WifiService {
  bool begin(String& localIp);
  bool beginMdns(AppState& state);
}
