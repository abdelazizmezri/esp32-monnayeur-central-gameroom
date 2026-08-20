#pragma once

namespace AppConfig {
  static const char* AP_SSID_PREFIX = "GAMEROOM-CENTRAL";
  static const char* AP_PASSWORD = "";
  static const char* MDNS_HOSTNAME = "pmm";
  static const char* DEFAULT_ADMIN_USERNAME = "admin";
  static const char* DEFAULT_ADMIN_PASSWORD = "12345678";
  // Must match PosteConfig::COMMAND_TOKEN on every poste ESP32.
  static const char* POSTE_COMMAND_TOKEN = "gameroom-poste-token-change-me";

  static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

  static const int SERVER_PORT = 80;

  // Le monnayeur est câblé sur le GPIO 13.
  static const int COIN_PIN = 13;

  static const int DEFAULT_COIN_DURATION_SECONDS = 1800;
  static const int DEFAULT_PULSES_PER_COIN = 1;
  static const int DEFAULT_AVAILABLE_COINS = 0;

  // Le JY-133B fournit une sortie impulsionnelle à collecteur ouvert, NO ou NC.
  // Une pull-down externe de 4,7 kΩ relie COIN/GPIO 13 au GND.
  // On valide une impulsion complète pour ne pas créditer les fronts parasites.
  static const unsigned long COIN_MIN_PULSE_MS = 10;
  static const unsigned long COIN_MAX_PULSE_MS = 250;
  static const unsigned long COIN_DEBOUNCE_MS = 80;
  static const unsigned long POST_REFRESH_INTERVAL_MS = 5000;
  static const unsigned long POST_OFFLINE_TIMEOUT_MS = 10000;
  static const unsigned long POSTE_STATUS_TIMEOUT_MS = 1200;
  // Une commande peut arriver pendant que le poste attend la réponse à son
  // annonce HTTP. Ce délai doit donc être supérieur au timeout de l'annonce.
  static const unsigned long POSTE_COMMAND_TIMEOUT_MS = 4000;
  static const unsigned long PENDING_POST_TIMEOUT_MS = 30000;
}
