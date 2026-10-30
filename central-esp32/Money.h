#pragma once

#include <stdint.h>
#include <stddef.h>

// Calculs financiers du firmware.
namespace Money {
enum Currency : uint8_t { TND = 0, DZD = 1 };
inline const char* code(uint8_t currency) { return currency == DZD ? "DZD" : "TND"; }
inline bool validValue(uint8_t currency, uint32_t minor) {
  if (currency == TND) return minor == 500 || minor == 1000 || minor == 2000 || minor == 5000;
  if (currency != DZD) return false;
  // Sélection pour le monnayeur de salle de jeux : 5 / 10 / 20 / 50 / 100 / 200 DA.
  // Les montants restent stockés en centimes algériens.
  return minor == 500 || minor == 1000 || minor == 2000 || minor == 5000 ||
         minor == 10000 || minor == 20000;
}
// Préserver les anciens réglages sans changer leur valeur au démarrage.
inline bool validStoredValue(uint8_t currency, uint32_t minor) {
  return validValue(currency, minor) ||
         (currency == DZD && (minor == 25 || minor == 50 || minor == 100 || minor == 200));
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
