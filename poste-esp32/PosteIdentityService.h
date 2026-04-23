#pragma once

#include "PosteState.h"

namespace PosteIdentityService {
  void begin(PosteState& state);
  bool saveIdentity(PosteState& state, const String& id, const String& name);
}
