#include "RelayService.h"
#include "PosteConfig.h"

#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <limits.h>

namespace RelayService {
  static Preferences sessionPrefs;
  static SemaphoreHandle_t sessionMutex = nullptr;

  static bool relayState = false;
  static unsigned long endTimeMs = 0;
  static bool recoveryPending = false;
  static unsigned long recoveryRemainingSeconds = 0;
  static unsigned long lastCheckpointAtMs = 0;
  static uint32_t sessionRevision = 0;

  class SessionLock {
   public:
    SessionLock()
        : locked(sessionMutex == nullptr ||
                 xSemaphoreTake(sessionMutex, portMAX_DELAY) == pdTRUE) {}

    ~SessionLock() {
      if (locked && sessionMutex != nullptr) {
        xSemaphoreGive(sessionMutex);
      }
    }

    bool acquired() const {
      return locked;
    }

   private:
    bool locked;
  };

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

  static unsigned long getRemainingSecondsUnlocked() {
    if (!relayState) return 0;

    unsigned long remainingMs = remainingMillis(endTimeMs);
    if (remainingMs == 0) return 0;

    return (remainingMs + 999UL) / 1000UL;
  }

  static void writeRelayHardware(bool on) {
    bool level = PosteConfig::RELAY_ACTIVE_HIGH ? on : !on;
    digitalWrite(PosteConfig::RELAY_PIN, level ? HIGH : LOW);
  }

  static void setRelayUnlocked(bool on) {
    relayState = on;
    writeRelayHardware(on);
  }

  static void clearPersistedSessionUnlocked() {
    sessionPrefs.begin("session", false);
    sessionPrefs.clear();
    sessionPrefs.end();
  }

  static void persistActiveSessionUnlocked() {
    unsigned long remainingSeconds = getRemainingSecondsUnlocked();
    if (!relayState || remainingSeconds == 0) {
      clearPersistedSessionUnlocked();
      return;
    }

    sessionPrefs.begin("session", false);
    sessionPrefs.putULong("remaining", remainingSeconds);
    sessionPrefs.end();
    lastCheckpointAtMs = millis();
  }

  static void startSessionUnlocked(unsigned long durationSeconds) {
    recoveryPending = false;
    recoveryRemainingSeconds = 0;
    endTimeMs = millis() + secondsToMillis(durationSeconds);
    setRelayUnlocked(true);
    ++sessionRevision;
    persistActiveSessionUnlocked();
  }

  static void stopSessionUnlocked() {
    bool stateChanged = relayState || endTimeMs != 0 || recoveryPending ||
                        recoveryRemainingSeconds != 0;

    endTimeMs = 0;
    recoveryPending = false;
    recoveryRemainingSeconds = 0;
    setRelayUnlocked(false);
    clearPersistedSessionUnlocked();

    if (stateChanged) {
      ++sessionRevision;
    }
  }

  bool begin() {
    sessionMutex = xSemaphoreCreateMutex();

    pinMode(PosteConfig::RELAY_PIN, OUTPUT);
    setRelayUnlocked(false);

    sessionPrefs.begin("session", true);
    unsigned long savedRemaining = sessionPrefs.getULong("remaining", 0);
    sessionPrefs.end();

    endTimeMs = 0;
    recoveryPending = savedRemaining > 0;
    recoveryRemainingSeconds = recoveryPending ? savedRemaining : 0;
    lastCheckpointAtMs = millis();
    sessionRevision = 1;

    if (recoveryPending) {
      Serial.print("Interrupted session detected. Recoverable time: ");
      Serial.print(recoveryRemainingSeconds);
      Serial.println(" sec");
    }

    return sessionMutex != nullptr;
  }

  void startSession(unsigned long durationSeconds) {
    SessionLock lock;
    if (!lock.acquired()) return;

    startSessionUnlocked(durationSeconds);

    Serial.print("START_SESSION: ");
    Serial.print(durationSeconds);
    Serial.println(" sec");
  }

  void addTime(unsigned long durationSeconds) {
    SessionLock lock;
    if (!lock.acquired()) return;

    if (!relayState) {
      startSessionUnlocked(durationSeconds);
    } else {
      unsigned long baseTime = hasReached(endTimeMs) ? millis() : endTimeMs;
      endTimeMs = baseTime + secondsToMillis(durationSeconds);
      recoveryPending = false;
      recoveryRemainingSeconds = 0;
      ++sessionRevision;
      persistActiveSessionUnlocked();
    }

    Serial.print("ADD_TIME: ");
    Serial.print(durationSeconds);
    Serial.println(" sec");
  }

  void stopSession() {
    SessionLock lock;
    if (!lock.acquired()) return;

    stopSessionUnlocked();
    Serial.println("STOP_SESSION");
  }

  void update() {
    SessionLock lock;
    if (!lock.acquired()) return;

    if (relayState && hasReached(endTimeMs)) {
      stopSessionUnlocked();
      Serial.println("Session finished automatically");
      return;
    }

    if (relayState &&
        millis() - lastCheckpointAtMs >= PosteConfig::SESSION_CHECKPOINT_INTERVAL_MS) {
      persistActiveSessionUnlocked();
    }
  }

  SessionSnapshot getSessionSnapshot() {
    SessionLock lock;
    SessionSnapshot snapshot;
    if (!lock.acquired()) return snapshot;

    snapshot.status = recoveryPending
                          ? SessionStatus::RecoveryPending
                          : (relayState ? SessionStatus::Active : SessionStatus::Idle);
    snapshot.relayState = relayState;
    snapshot.remainingSeconds = getRemainingSecondsUnlocked();
    snapshot.recoveryPending = recoveryPending;
    snapshot.recoveryRemainingSeconds = recoveryRemainingSeconds;
    snapshot.revision = sessionRevision;
    return snapshot;
  }

  unsigned long getRemainingSeconds() {
    return getSessionSnapshot().remainingSeconds;
  }

  const char* statusToString(SessionStatus status) {
    switch (status) {
      case SessionStatus::Active:
        return "active";
      case SessionStatus::RecoveryPending:
        return "recovery_pending";
      case SessionStatus::Idle:
      default:
        return "idle";
    }
  }
}
