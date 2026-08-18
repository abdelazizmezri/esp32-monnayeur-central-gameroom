#include "CoinService.h"
#include "AppConfig.h"
#include "StorageService.h"
#include "LogService.h"

#include <Arduino.h>

static AppState* gCoinState = nullptr;
static volatile bool gHasAcceptedPulse = false;
static volatile int gCoinIdleLevel = HIGH;
static volatile int gCoinCurrentLevel = HIGH;
static volatile unsigned long gCoinLevelStartedAt = 0;
static volatile unsigned long gLastAcceptedPulseAt = 0;

void IRAM_ATTR coinInterruptHandler() {
  CoinService::onInterrupt();
}

namespace CoinService {
  void begin(AppState& state) {
    gCoinState = &state;
    gHasAcceptedPulse = false;
    gLastAcceptedPulseAt = 0;

    // La sortie COIN est maintenue à LOW par une pull-down externe de 4,7 kΩ.
    pinMode(AppConfig::COIN_PIN, INPUT);
    gCoinCurrentLevel = digitalRead(AppConfig::COIN_PIN);
    gCoinIdleLevel = gCoinCurrentLevel;
    gCoinLevelStartedAt = millis();
    attachInterrupt(digitalPinToInterrupt(AppConfig::COIN_PIN), coinInterruptHandler, CHANGE);
  }

  void IRAM_ATTR onInterrupt() {
    if (!gCoinState) {
      return;
    }

    unsigned long now = millis();
    int newLevel = digitalRead(AppConfig::COIN_PIN);

    if (newLevel == gCoinCurrentLevel) {
      return;
    }

    int previousLevel = gCoinCurrentLevel;
    unsigned long levelDuration = now - gCoinLevelStartedAt;
    gCoinCurrentLevel = newLevel;
    gCoinLevelStartedAt = now;

    // Le niveau qui reste présent plus longtemps qu'une impulsion est le niveau
    // de repos. Cela permet de prendre en charge automatiquement les sorties
    // normalement ouvertes (impulsion LOW) et normalement fermées (impulsion HIGH).
    if (levelDuration > AppConfig::COIN_MAX_PULSE_MS) {
      gCoinIdleLevel = previousLevel;
      return;
    }

    if (previousLevel == gCoinIdleLevel ||
        levelDuration < AppConfig::COIN_MIN_PULSE_MS) {
      return;
    }

    if (gHasAcceptedPulse &&
        now - gLastAcceptedPulseAt < AppConfig::COIN_DEBOUNCE_MS) {
      return;
    }

    gCoinState->pulseCount++;
    gCoinState->lastInterruptTime = now;
    gLastAcceptedPulseAt = now;
    gHasAcceptedPulse = true;
  }

  void update(AppState& state) {
    static int lastProcessedPulses = 0;

    if (state.pulsesPerCoin <= 0) {
      state.pulsesPerCoin = AppConfig::DEFAULT_PULSES_PER_COIN;
      LogService::warn(state, "Configuration pulsesPerCoin invalide, valeur par défaut appliquée.");
      StorageService::saveConfig(state);
    }

    noInterrupts();
    int currentPulses = state.pulseCount;
    interrupts();

    int diff = currentPulses - lastProcessedPulses;
    if (diff >= state.pulsesPerCoin) {
      int newCoins = diff / state.pulsesPerCoin;
      state.availableCoins += newCoins;
      lastProcessedPulses += newCoins * state.pulsesPerCoin;

      StorageService::saveCredits(state);
      LogService::info(state, "Nouveaux coins reçus: +" + String(newCoins) + ", total=" + String(state.availableCoins));
    }
  }
}
