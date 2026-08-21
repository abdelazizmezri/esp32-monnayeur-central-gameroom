#pragma once

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
inline constexpr int DEFAULT_AVAILABLE_COINS = 0;

// Le JY-133B fournit une sortie impulsionnelle à collecteur ouvert, NO ou NC.
// Une pull-down externe de 4,7 kΩ relie COIN/GPIO 13 au GND.
// On valide une impulsion complète pour ne pas créditer les fronts parasites.
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
