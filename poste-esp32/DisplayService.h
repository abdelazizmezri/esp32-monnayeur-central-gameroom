#pragma once

#include "RelayService.h"

namespace DisplayService {
  void begin(const RelayService::SessionSnapshot& session);
  void update(const RelayService::SessionSnapshot& session);
}
