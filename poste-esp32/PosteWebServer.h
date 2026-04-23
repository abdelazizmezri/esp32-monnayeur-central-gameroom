#pragma once

#include <WebServer.h>
#include "PosteState.h"

namespace PosteWebServer {
  void begin(WebServer& server, PosteState& state);
}