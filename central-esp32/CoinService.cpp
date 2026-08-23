#include "CoinService.h"
#include "AppConfig.h"
#include "StorageService.h"
#include "LogService.h"
#include "RevenueService.h"

#include <Arduino.h>

static AppState* gCoinState = nullptr;
static volatile bool gHasAcceptedPulse = false;
static volatile int gCoinIdleLevel = HIGH;
static volatile int gCoinCurrentLevel = HIGH;
static volatile unsigned long gCoinLevelStartedAt = 0;
static volatile unsigned long gLastAcceptedPulseAt = 0;
static int gLastObservedPulses = 0;

static void IRAM_ATTR coinInterruptHandler() {
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

  gCoinState->pulseCount = gCoinState->pulseCount + 1;
  gLastAcceptedPulseAt = now;
  gHasAcceptedPulse = true;
}

namespace CoinService {
  void begin(AppState& state) {
    gCoinState = &state;
    gHasAcceptedPulse = false;
    gLastAcceptedPulseAt = 0;
    gLastObservedPulses = 0;
    state.pulseCount = 0;

    // Le circuit d'interface externe maintient un niveau logique stable au
    // repos. La polarité NO/NC est ensuite apprise automatiquement.
    pinMode(AppConfig::COIN_PIN, INPUT);
    gCoinCurrentLevel = digitalRead(AppConfig::COIN_PIN);
    gCoinIdleLevel = gCoinCurrentLevel;
    gCoinLevelStartedAt = millis();
    attachInterrupt(digitalPinToInterrupt(AppConfig::COIN_PIN), coinInterruptHandler, CHANGE);
  }

  void update(AppState& state) {
    if (state.pulsesPerCoin <= 0) {
      state.pulsesPerCoin = AppConfig::DEFAULT_PULSES_PER_COIN;
      LogService::warn(state, "Configuration pulsesPerCoin invalide, valeur par défaut appliquée.");
      StorageService::saveConfig(state);
    }

    noInterrupts();
    int currentPulses = state.pulseCount;
    interrupts();

    int diff = currentPulses - gLastObservedPulses;
    if (diff > 0) {
      gLastObservedPulses = currentPulses;
      int groupedPulses = state.pendingPulses + diff;
      int newCoins = groupedPulses / state.pulsesPerCoin;
      state.pendingPulses = groupedPulses % state.pulsesPerCoin;
      state.availableCoins += newCoins;

      StorageService::saveCredits(state);
      bool stored = RevenueService::recordPulses(state, diff, newCoins);
      String message = "Impulsions monnayeur validées: +" + String(diff) +
                       ", recette +" +
                       String((double)diff * state.pulseValueMillimes / 1000.0, 3) +
                       " TND, coins +" + String(newCoins) +
                       ", total=" + String(state.availableCoins);
      if (stored) {
        LogService::info(state, message);
      } else {
        LogService::error(state, message + " (échec stockage LittleFS)");
      }
    }
  }
}
