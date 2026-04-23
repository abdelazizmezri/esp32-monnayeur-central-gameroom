#pragma once

#include <WebServer.h>
#include "AppState.h"

namespace WebRoutes {
  void registerRoutes(WebServer& server, AppState& state);
}