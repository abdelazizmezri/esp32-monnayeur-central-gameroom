#include "CoinService.h"
#include "AppConfig.h"
#include "StorageService.h"
#include "LogService.h"
#include "RevenueService.h"

#include <Arduino.h>
#include <time.h>

static AppState* gCoinState = nullptr;
static volatile bool gCoinArmed = false;
static uint32_t gCoinStartedAt = 0;
static volatile bool gHasAcceptedPulse = false;
static volatile uint32_t gLastAcceptedPulseAt = 0;
namespace {
struct Pulse {
  uint8_t currency;
  uint32_t value, rate;
  uint32_t epoch;
};
constexpr uint16_t QUEUE_SIZE = 128;
Pulse gQueue[QUEUE_SIZE];
Pulse gActive;
volatile uint16_t gRead = 0, gWrite = 0;
volatile uint32_t gOverflow = 0;
volatile uint32_t gEpoch = 0, gEpochMillis = 0;
uint32_t gLastFailureLog = 0;
uint64_t gInFlightSequence = 0;
uint16_t gInFlightEnd = 0;
bool sameProfile(const Pulse& a, const Pulse& b) {
  return a.currency == b.currency && a.value == b.value && a.rate == b.rate && a.epoch == b.epoch;
}
}

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
  uint16_t next = (gWrite+1) % QUEUE_SIZE;
  if (next == gRead) { ++gOverflow; return; }
  // Seulement une capture RAM : aucune allocation, NVS ni LittleFS dans l'ISR.
  Pulse& item = gQueue[gWrite];
  item.currency = gActive.currency; item.value = gActive.value;
  item.rate = gActive.rate;
  item.epoch = gEpoch ? gEpoch + (uint32_t)(now-gEpochMillis)/1000 : 0;
  gWrite = next;
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
    gRead = gWrite = 0;
    gOverflow = 0;
    gInFlightSequence = 0;
    publishConfiguration(state);
    state.pulseCount = 0;

    // Comme l'entrée active LOW du projet de référence. Le signal doit rester
    // compatible 3,3 V, sans résistance externe de rappel vers GND.
    pinMode(AppConfig::COIN_PIN, INPUT_PULLUP);
    int initialLevel = digitalRead(AppConfig::COIN_PIN);
    attachInterrupt(digitalPinToInterrupt(AppConfig::COIN_PIN), coinInterruptHandler, FALLING);
    LogService::info(state, "Monnayeur en stabilisation: GPIO" + String(AppConfig::COIN_PIN) +
                     ", niveau initial=" + String(initialLevel == HIGH ? "HIGH" : "LOW") +
                     ", impulsions ignorées pendant " + String(AppConfig::COIN_STARTUP_IGNORE_MS) +
                     " ms, solde initial=" + String(state.availablePieces));
  }

  static bool armIfReady(AppState& state) {
    if (gCoinArmed) {
      return true;
    }
    if (!state.creditStorageReady || !state.revenueStorageReady || state.revenueTransactionPending) return false;
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
                     " ms, 1 impulsion = 1 pièce disponible");
    return true;
  }

  void publishConfiguration(const AppState& state) {
    noInterrupts();
    gActive.currency = state.coinCurrency; gActive.value = state.pulseValueMinor;
    gActive.rate = state.dzdToTndRateMicros;
    interrupts();
  }

  void update(AppState& state) {
    time_t now = time(nullptr);
    noInterrupts();
    gEpoch = state.revenueTimeReady ? (uint32_t)now : 0;
    gEpochMillis = millis();
    uint32_t overflow = gOverflow; gOverflow = 0;
    interrupts();
    if (overflow) LogService::error(state,"File monnayeur saturée : " + String(overflow) +
      " impulsion(s) non sauvegardée(s). Vérifier le stockage avant de continuer.");
    if (!RevenueService::recoverPending(state)) {
      if (millis()-gLastFailureLog >= 5000) {
        LogService::error(state,state.revenueStorageError); gLastFailureLog = millis();
      }
      return;
    }
    // Une préparation peut avoir été engagée même si sa relecture a échoué.
    // Reconnaître sa séquence avant de retenter la même impulsion RAM.
    if (gInFlightSequence) {
      if (state.lastRevenueSequence >= gInFlightSequence) {
        noInterrupts(); gRead = gInFlightEnd; interrupts();
      }
      gInFlightSequence = 0;
    }
    if (!armIfReady(state)) return;

    // Borné pour laisser le serveur et les postes s'exécuter entre deux lots.
    for (unsigned batch = 0; batch < 8; ++batch) {
      noInterrupts();
      uint16_t read = gRead, end = gWrite;
      interrupts();
      if (read == end) return;
      Pulse profile = gQueue[read];
      int count = 1;
      uint16_t next = (read+1) % QUEUE_SIZE;
      while (next != end && sameProfile(profile,gQueue[next])) {
        ++count; next = (next+1) % QUEUE_SIZE;
      }
      gInFlightSequence = state.lastRevenueSequence+1; gInFlightEnd = next;
      if (!RevenueService::recordPulses(state,count,profile.currency,profile.value,
                                       profile.rate,profile.epoch)) {
        if (millis()-gLastFailureLog >= 5000) {
          LogService::error(state,state.revenueStorageError); gLastFailureLog = millis();
        }
        return;
      }
      // La préparation NVS est durable ; une éventuelle reprise ne relira pas la file.
      noInterrupts(); gRead = next; interrupts();
      gInFlightSequence = 0;
      LogService::info(state,"Impulsions monnayeur validées: +" + String(count) +
        ", recette +" + RevenueService::formatMinor((uint64_t)count*profile.value,
          profile.currency == Money::DZD ? 2 : 3) + " " + Money::code(profile.currency) +
        ", solde=" + String(state.availablePieces));
      if (state.revenueTransactionPending) return;
    }
  }

  bool settle(AppState& state) {
    update(state);
    noInterrupts(); bool empty = gRead == gWrite; interrupts();
    return empty && !state.revenueTransactionPending && state.creditStorageReady && state.revenueStorageReady;
  }
}
