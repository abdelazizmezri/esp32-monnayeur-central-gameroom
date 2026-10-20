#pragma once

#include <stdint.h>
#include <stddef.h>

// Calculs financiers du firmware.
namespace Money {
enum Currency : uint8_t { TND = 0, DZD = 1 };
constexpr uint32_t RATE_SCALE = 1000000;  // micro-TND pour 1 DZD
constexpr uint32_t MAX_RATE = 1000000000; // 1000 TND pour 1 DZD

inline const char* code(uint8_t currency) { return currency == DZD ? "DZD" : "TND"; }
inline bool validValue(uint8_t currency, uint32_t minor) {
  if (currency == TND) return minor == 500 || minor == 1000 || minor == 2000 || minor == 5000;
  if (currency != DZD) return false;
  // Centimes algériens : 0,25 / 0,50 / 1 / 2 / 5 / 10 / 20 / 50 / 100 / 200 DZD.
  return minor == 25 || minor == 50 || minor == 100 || minor == 200 ||
         minor == 500 || minor == 1000 || minor == 2000 || minor == 5000 ||
         minor == 10000 || minor == 20000;
}
inline bool validRate(uint32_t rate) { return rate > 0 && rate <= MAX_RATE; }
inline uint64_t toMillimes(uint8_t currency, uint32_t minor, uint32_t rate) {
  // Arrondi au millime le plus proche, demi vers le haut, par impulsion.
  // Une agrégation conserve la somme des conversions déjà arrondies.
  return currency == TND ? minor : ((uint64_t)minor * rate + 50000) / 100000;
}
inline bool parseRate(const char* text, uint32_t& result) {
  if (!text || !*text) return false;
  uint64_t whole = 0, fraction = 0;
  unsigned digits = 0;
  bool decimal = false, any = false;
  for (const char* p = text; *p; ++p) {
    if ((*p == '.' || *p == ',') && !decimal) { decimal = true; continue; }
    if (*p < '0' || *p > '9') return false;
    any = true;
    if (decimal) {
      if (++digits > 6) return false;
      fraction = fraction * 10 + (*p - '0');
    } else {
      whole = whole * 10 + (*p - '0');
      if (whole > 1000) return false;
    }
  }
  while (digits++ < 6) fraction *= 10;
  uint64_t rate = whole * RATE_SCALE + fraction;
  if (!any || rate == 0 || rate > MAX_RATE) return false;
  result = (uint32_t)rate;
  return true;
}
inline uint16_t crc16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xffff;
  for (size_t i = 0; i < length; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; ++b)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}
}
