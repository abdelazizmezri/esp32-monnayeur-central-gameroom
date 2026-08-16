#include "RelayService.h"
#include "PosteConfig.h"

#include <Arduino.h>
#include <Preferences.h>
#include <limits.h>

namespace RelayService {
  static Preferences sessionPrefs;
  static unsigned long lastCheckpointAtMs = 0;

  static unsigned long secondsToMillis(unsigned long durationSeconds) {
    if (durationSeconds > ULONG_MAX / 1000UL) {
      return ULONG_MAX;
    }

    return durationSeconds * 1000UL;
  }

  static bool hasReached(unsigned long targetMs) {
    return (long)(millis() - targetMs) >= 0;
  }

  static unsigned long remainingMillis(unsigned long targetMs) {
    long remaining = (long)(targetMs - millis());
    return remaining > 0 ? (unsigned long)remaining : 0;
  }

  static void writeRelayHardware(bool on) {
    bool level = PosteConfig::RELAY_ACTIVE_HIGH ? on : !on;
    digitalWrite(PosteConfig::RELAY_PIN, level ? HIGH : LOW);
  }

  static void clearPersistedSession() {
    sessionPrefs.begin("session", false);
    sessionPrefs.clear();
    sessionPrefs.end();
  }

  static void persistActiveSession(const PosteState& state) {
    unsigned long remainingSeconds = getRemainingSeconds(state);
    if (!state.relayState || remainingSeconds == 0) {
      clearPersistedSession();
      return;
    }

    sessionPrefs.begin("session", false);
    sessionPrefs.putULong("remaining", remainingSeconds);
    sessionPrefs.end();
    lastCheckpointAtMs = millis();
  }

  void begin(PosteState& state) {
    pinMode(PosteConfig::RELAY_PIN, OUTPUT);
    setRelay(state, false);

    sessionPrefs.begin("session", true);
    unsigned long savedRemaining = sessionPrefs.getULong("remaining", 0);
    sessionPrefs.end();

    state.endTimeMs = 0;
    state.recoveryPending = savedRemaining > 0;
    state.recoveryRemainingSeconds = state.recoveryPending ? savedRemaining : 0;
    state.status = state.recoveryPending ? "recovery_pending" : "idle";

    if (state.recoveryPending) {
      Serial.print("Interrupted session detected. Recoverable time: ");
      Serial.print(state.recoveryRemainingSeconds);
      Serial.println(" sec");
    }
  }

  void setRelay(PosteState& state, bool on) {
    state.relayState = on;
    writeRelayHardware(on);
  }

  long getRemainingSeconds(const PosteState& state) {
    if (!state.relayState) return 0;

    unsigned long remainingMs = remainingMillis(state.endTimeMs);
    if (remainingMs == 0) return 0;

    return (remainingMs + 999UL) / 1000UL;
  }

  void startSession(PosteState& state, unsigned long durationSeconds) {
    state.recoveryPending = false;
    state.recoveryRemainingSeconds = 0;
    state.endTimeMs = millis() + secondsToMillis(durationSeconds);
    state.status = "active";
    setRelay(state, true);
    persistActiveSession(state);

    Serial.print("START_SESSION: ");
    Serial.print(durationSeconds);
    Serial.println(" sec");
  }

  void addTime(PosteState& state, unsigned long durationSeconds) {
    if (!state.relayState) {
      startSession(state, durationSeconds);
      return;
    }

    unsigned long baseTime = hasReached(state.endTimeMs) ? millis() : state.endTimeMs;
    state.endTimeMs = baseTime + secondsToMillis(durationSeconds);
    state.status = "active";
    state.recoveryPending = false;
    state.recoveryRemainingSeconds = 0;
    persistActiveSession(state);

    Serial.print("ADD_TIME: ");
    Serial.print(durationSeconds);
    Serial.println(" sec");
  }

  void stopSession(PosteState& state) {
    state.endTimeMs = 0;
    state.status = "idle";
    state.recoveryPending = false;
    state.recoveryRemainingSeconds = 0;
    setRelay(state, false);
    clearPersistedSession();

    Serial.println("STOP_SESSION");
  }

  void update(PosteState& state) {
    if (state.relayState && hasReached(state.endTimeMs)) {
      stopSession(state);
      Serial.println("Session finished automatically");
      return;
    }

    if (state.relayState &&
        millis() - lastCheckpointAtMs >= PosteConfig::SESSION_CHECKPOINT_INTERVAL_MS) {
      persistActiveSession(state);
    }
  }
}
