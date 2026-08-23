#pragma once

#include <Arduino.h>

namespace RelayService {
  enum class SessionStatus : uint8_t {
    Idle,
    Active,
    RecoveryPending,
  };

  struct SessionSnapshot {
    SessionStatus status = SessionStatus::Idle;
    bool relayState = false;
    unsigned long remainingSeconds = 0;
    bool recoveryPending = false;
    unsigned long recoveryRemainingSeconds = 0;
    uint32_t revision = 0;
  };

  // Retourne false uniquement si le mutex FreeRTOS n'a pas pu etre cree. Le
  // service reste alors utilisable depuis la boucle Arduino de secours.
  bool begin();
  void update();

  void startSession(unsigned long durationSeconds);
  void addTime(unsigned long durationSeconds);
  void stopSession();

  SessionSnapshot getSessionSnapshot();
  unsigned long getRemainingSeconds();
  const char* statusToString(SessionStatus status);
}
