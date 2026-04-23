#include "CoinService.h"
#include "AppConfig.h"
#include "StorageService.h"
#include "LogService.h"

#include <Arduino.h>

static AppState* gCoinState = nullptr;

void IRAM_ATTR coinInterruptHandler() {
  CoinService::onInterrupt();
}

namespace CoinService {
  void begin(AppState& state) {
    gCoinState = &state;

    pinMode(AppConfig::COIN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(AppConfig::COIN_PIN), coinInterruptHandler, FALLING);
  }

  void onInterrupt() {
    if (!gCoinState) {
      return;
    }

    unsigned long now = millis();
    if (now - gCoinState->lastInterruptTime > AppConfig::COIN_DEBOUNCE_MS) {
      gCoinState->pulseCount++;
      gCoinState->lastInterruptTime = now;
    }
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
