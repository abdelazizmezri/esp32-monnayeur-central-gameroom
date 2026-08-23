#pragma once

#include <stdint.h>

namespace AppConfig {
inline constexpr char AP_SSID_PREFIX[] = "GAMEROOM-CENTRAL";
inline constexpr char AP_PASSWORD[] = "";
inline constexpr char MDNS_HOSTNAME[] = "pstm";
inline constexpr char DEFAULT_ADMIN_USERNAME[] = "admin";
inline constexpr char DEFAULT_ADMIN_PASSWORD[] = "12345678";
// Doit correspondre à PosteConfig::COMMAND_TOKEN sur chaque poste.
inline constexpr char POSTE_COMMAND_TOKEN[] = "gameroom-poste-token-change-me";

inline constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

inline constexpr int SERVER_PORT = 80;

// Le monnayeur est câblé sur le GPIO 13.
inline constexpr int COIN_PIN = 13;

inline constexpr int DEFAULT_COIN_DURATION_SECONDS = 1800;
inline constexpr int DEFAULT_PULSES_PER_COIN = 1;
// Valeur monétaire d'une impulsion validée, en millimes tunisiens.
// 1000 millimes = 1 TND.
inline constexpr int DEFAULT_PULSE_VALUE_MILLIMES = 1000;
inline constexpr int DEFAULT_AVAILABLE_COINS = 0;

// Journal financier binaire compact dans la partition LittleFS.
inline constexpr char REVENUE_FILE[] = "/revenue.bin";
inline constexpr unsigned long REVENUE_STORAGE_RETRY_INTERVAL_MS = 30000;

// Heure locale tunisienne (UTC+1, sans changement saisonnier actuel).
inline constexpr char TIMEZONE[] = "CET-1";
inline constexpr char NTP_SERVER_1[] = "pool.ntp.org";
inline constexpr char NTP_SERVER_2[] = "time.nist.gov";

// Le JY-133B fournit une sortie impulsionnelle NO ou NC. Le GPIO doit recevoir
// un signal conditionné en 3,3 V (optocoupleur recommandé). On valide une
// impulsion complète pour ne pas créditer les fronts parasites.
inline constexpr unsigned long COIN_MIN_PULSE_MS = 10;
inline constexpr unsigned long COIN_MAX_PULSE_MS = 250;
inline constexpr unsigned long COIN_DEBOUNCE_MS = 80;
inline constexpr unsigned long POST_REFRESH_INTERVAL_MS = 5000;
inline constexpr unsigned long POST_OFFLINE_TIMEOUT_MS = 10000;
inline constexpr unsigned long POSTE_STATUS_TIMEOUT_MS = 1200;
// Une commande peut arriver pendant que le poste attend la réponse à son
// annonce HTTP. Ce délai doit donc être supérieur au timeout de l'annonce.
inline constexpr unsigned long POSTE_COMMAND_TIMEOUT_MS = 4000;
inline constexpr unsigned long PENDING_POST_TIMEOUT_MS = 30000;
}
