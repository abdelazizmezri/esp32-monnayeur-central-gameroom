#pragma once

#include <WebServer.h>
#include "PosteState.h"

namespace WiFiConfigService {
  void begin(WebServer& server, PosteState& state);
  void loop(PosteState& state);
  bool isConnected(const PosteState& state);
}
