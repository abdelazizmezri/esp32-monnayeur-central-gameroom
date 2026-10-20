#include "CoinService.h"
#include "AppConfig.h"
#include "StorageService.h"
#include "LogService.h"
#include "RevenueService.h"

#include <Arduino.h>

static AppState* gCoinState = nullptr;
static volatile bool gCoinArmed = false;
static uint32_t gCoinStartedAt = 0;
static volatile bool gHasAcceptedPulse = false;
static volatile uint32_t gLastAcceptedPulseAt = 0;
static int gLastObservedPulses = 0;

static void IRAM_ATTR coinInterruptHandler() {
  if (!gCoinState || !gCoinArmed || digitalRead(AppConfig::COIN_PIN) != LOW) {
    return;
  }

  uint32_t now = (uint32_t)millis();
  // Le verrouillage part du front accepté : les doublons ne le prolongent pas.
  if (gHasAcceptedPulse &&
      now - gLastAcceptedPulseAt < AppConfig::COIN_DEBOUNCE_MS) {
    return;
  }

  // Un front descendant = une impulsion, sans attendre son retour à HIGH.
  // Le stockage, la recette et les logs restent dans la boucle principale.
  gCoinState->pulseCount = gCoinState->pulseCount + 1;
  gLastAcceptedPulseAt = now;
  gHasAcceptedPulse = true;
}

namespace CoinService {
  void begin(AppState& state) {
    gCoinArmed = false;
    gCoinStartedAt = (uint32_t)millis();
    gCoinState = &state;
    gHasAcceptedPulse = false;
    gLastAcceptedPulseAt = 0;
    gLastObservedPulses = 0;
    state.pulseCount = 0;

    // Comme l'entrée active LOW du projet de référence. Le signal doit rester
    // compatible 3,3 V, sans résistance externe de rappel vers GND.
    pinMode(AppConfig::COIN_PIN, INPUT_PULLUP);
    int initialLevel = digitalRead(AppConfig::COIN_PIN);
    attachInterrupt(digitalPinToInterrupt(AppConfig::COIN_PIN), coinInterruptHandler, FALLING);
    LogService::info(state, "Monnayeur en stabilisation: GPIO" + String(AppConfig::COIN_PIN) +
                     ", niveau initial=" + String(initialLevel == HIGH ? "HIGH" : "LOW") +
                     ", impulsions ignorées pendant " + String(AppConfig::COIN_STARTUP_IGNORE_MS) +
                     " ms, solde initial=" + String(state.availableCoins));
  }

  static bool armIfReady(AppState& state) {
    if (gCoinArmed) {
      return true;
    }
    uint32_t now = (uint32_t)millis();
    if (now - gCoinStartedAt < AppConfig::COIN_STARTUP_IGNORE_MS) {
      return false;
    }

    // Armer seulement au repos HIGH : une ligne encore LOW pendant le boot
    // ne doit jamais devenir une pièce. Un front en attente est aussi ignoré
    // par l'ISR si la broche est déjà revenue à HIGH.
    noInterrupts();
    bool idle = digitalRead(AppConfig::COIN_PIN) == HIGH;
    if (idle) {
      gCoinArmed = true;
    }
    interrupts();
    if (!idle) {
      return false;
    }

    LogService::info(state, "Monnayeur prêt: GPIO" + String(AppConfig::COIN_PIN) +
                     ", rappel=pull-up interne, détection=front descendant" +
                     ", repos=HIGH" +
                     ", anti-doublon=" + String(AppConfig::COIN_DEBOUNCE_MS) +
                     " ms, impulsions/coin=" + String(state.pulsesPerCoin));
    return true;
  }

  void update(AppState& state) {
    if (state.pulsesPerCoin <= 0) {
      state.pulsesPerCoin = AppConfig::DEFAULT_PULSES_PER_COIN;
      LogService::warn(state, "Configuration pulsesPerCoin invalide, valeur par défaut appliquée.");
      StorageService::saveConfig(state);
    }

    if (!armIfReady(state)) {
      return;
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
