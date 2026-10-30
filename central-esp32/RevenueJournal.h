#pragma once

#include "Money.h"
#include <string.h>

namespace RevenueJournal {
constexpr size_t HEADER_SIZE = 16;
constexpr size_t RECORD_SIZE = 72;
constexpr uint8_t HEADER[HEADER_SIZE] = {'P','S','T','M','R','E','V','2',2,0,0,0,72,0,0,0};
constexpr uint8_t LEGACY_HEADER[HEADER_SIZE] = {'P','S','T','M','R','E','V','1',1,0,0,0,16,0,0,0};

struct Entry {
  uint32_t epoch = 0;
  uint8_t currency = Money::TND;
  uint32_t rate = 0; // Champ historique REV2 conservé, jamais utilisé pour les calculs.
  uint32_t sessionId = 0;
  uint32_t pulseValue = 0;
  uint64_t nativeAmount = 0;
  uint64_t convertedMillimes = 0;
  uint32_t pulses = 0;
  uint32_t credits = 0;
  uint64_t sequence = 0;
  uint32_t balanceAfter = 0;
  uint32_t pendingAfter = 0;
  uint32_t pulsesPerCoin = 1;
  uint32_t events = 1;
};
inline void put(uint8_t* p, uint64_t value, unsigned size) {
  for (unsigned i = 0; i < size; ++i) { p[i] = value & 255; value >>= 8; }
}
inline uint64_t get(const uint8_t* p, unsigned size) {
  uint64_t result = 0;
  for (unsigned i = 0; i < size; ++i) result |= (uint64_t)p[i] << (i * 8);
  return result;
}
inline void encode(const Entry& e, uint8_t* r) {
  memset(r, 0, RECORD_SIZE);
  put(r, e.epoch, 4); r[4] = e.currency;
  put(r+8, e.rate, 4); put(r+12, e.pulseValue, 4);
  put(r+16, e.nativeAmount, 8); put(r+24, e.convertedMillimes, 8);
  put(r+32, e.pulses, 4); put(r+36, e.credits, 4); put(r+40, e.sequence, 8);
  put(r+48, e.balanceAfter, 4); put(r+52, e.pendingAfter, 4);
  put(r+56, e.pulsesPerCoin, 4); put(r+60, e.events, 4);
  put(r+64, e.sessionId, 4);
  put(r+70, Money::crc16(r, 70), 2);
}
inline bool decode(const uint8_t* r, Entry& e) {
  if (get(r+70, 2) != Money::crc16(r, 70) || r[4] > Money::DZD) return false;
  e.sessionId = get(r+64,4);
  e.epoch = get(r,4); e.currency = r[4]; e.rate = get(r+8,4); e.pulseValue = get(r+12,4);
  e.nativeAmount = get(r+16,8); e.convertedMillimes = get(r+24,8);
  e.pulses = get(r+32,4); e.credits = get(r+36,4); e.sequence = get(r+40,8);
  e.balanceAfter = get(r+48,4); e.pendingAfter = get(r+52,4);
  e.pulsesPerCoin = get(r+56,4); e.events = get(r+60,4);
  return e.pulsesPerCoin > 0 && e.pendingAfter < e.pulsesPerCoin &&
         e.balanceAfter <= INT32_MAX && e.nativeAmount <= INT64_MAX &&
         e.convertedMillimes <= INT64_MAX;
}
inline bool decodeLegacy(const uint8_t* r, Entry& e) {
  if (get(r+14,2) != Money::crc16(r,14)) return false;
  e = Entry(); e.epoch = get(r,4); e.nativeAmount = get(r+4,4);
  e.convertedMillimes = e.nativeAmount; e.pulses = get(r+8,2); e.credits = get(r+10,2);
  return true;
}
inline bool sameBucket(const Entry& a, const Entry& b) {
  return a.epoch / 3600 == b.epoch / 3600 && (a.epoch == 0) == (b.epoch == 0) &&
         a.sessionId == b.sessionId && a.currency == b.currency && a.rate == b.rate && a.pulseValue == b.pulseValue;
}
inline bool merge(Entry& a, const Entry& b) {
  if (!sameBucket(a,b) || a.nativeAmount > INT64_MAX-b.nativeAmount ||
      a.convertedMillimes > INT64_MAX-b.convertedMillimes ||
      (uint64_t)a.pulses+b.pulses > UINT32_MAX ||
      (uint64_t)a.credits+b.credits > UINT32_MAX ||
      (uint64_t)a.events+b.events > UINT32_MAX) return false;
  a.nativeAmount += b.nativeAmount; a.convertedMillimes += b.convertedMillimes;
  a.pulses += b.pulses; a.credits += b.credits; a.events += b.events;
  if (b.sequence > a.sequence) {
    a.sequence = b.sequence; a.balanceAfter = b.balanceAfter;
    a.pendingAfter = b.pendingAfter; a.pulsesPerCoin = b.pulsesPerCoin;
  }
  // Ne pas transformer une recette datée en recette non datée.
  if (a.epoch) a.epoch = a.epoch / 3600 * 3600;
  return true;
}
}
