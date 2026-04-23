#include "PosteIdentityService.h"

#include "PosteConfig.h"

#include <Arduino.h>
#include <Preferences.h>

namespace PosteIdentityService {
  static Preferences prefs;

  static String buildChipId() {
    uint64_t mac = ESP.getEfuseMac();
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%04X%08X",
             (uint16_t)(mac >> 32),
             (uint32_t)mac);
    return String(buffer);
  }

  static String trimmedCopy(const String& value) {
    String trimmed = value;
    trimmed.trim();
    return trimmed;
  }

  void begin(PosteState& state) {
    state.chipId = buildChipId();

    prefs.begin("identity", true);
    state.id = prefs.getString("id", PosteConfig::DEFAULT_POST_ID);
    state.name = prefs.getString("name", PosteConfig::DEFAULT_POST_NAME);
    prefs.end();

    state.id.trim();
    state.name.trim();
    state.configured = !state.id.isEmpty() && !state.name.isEmpty();
  }

  bool saveIdentity(PosteState& state, const String& id, const String& name) {
    String cleanId = trimmedCopy(id);
    String cleanName = trimmedCopy(name);
    if (cleanId.isEmpty() || cleanName.isEmpty()) {
      return false;
    }

    prefs.begin("identity", false);
    prefs.putString("id", cleanId);
    prefs.putString("name", cleanName);
    prefs.end();

    state.id = cleanId;
    state.name = cleanName;
    state.configured = true;
    return true;
  }
}
