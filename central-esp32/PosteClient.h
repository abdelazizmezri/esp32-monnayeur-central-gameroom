#pragma once

#include "AppConfig.h"
#include "Post.h"
#include <Arduino.h>

namespace PosteClient {
  bool sendCommand(const String& ip, const String& action, long durationSeconds = 0);
  bool fetchStatus(Post& post, uint16_t timeoutMs = AppConfig::POSTE_STATUS_TIMEOUT_MS);
}
