#pragma once

namespace AppConfig {
  static const char* AP_SSID = "GameRoom-Setup";
  static const char* AP_PASSWORD = "12345678";
  static const char* MDNS_HOSTNAME = "gameroom";
  static const char* DEFAULT_ADMIN_PASSWORD = "admin1234";
  // Must match PosteConfig::COMMAND_TOKEN on every poste ESP32.
  static const char* POSTE_COMMAND_TOKEN = "gameroom-poste-token-change-me";

  static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

  static const int SERVER_PORT = 80;
  static const int COIN_PIN = 27;
  static const int DEFAULT_COIN_DURATION_SECONDS = 1800;
  static const int DEFAULT_PULSES_PER_COIN = 1;
  static const int DEFAULT_AVAILABLE_COINS = 0;

  static const unsigned long COIN_DEBOUNCE_MS = 80;
  static const unsigned long POST_REFRESH_INTERVAL_MS = 5000;
  static const unsigned long POST_OFFLINE_TIMEOUT_MS = 10000;
  static const unsigned long POSTE_STATUS_TIMEOUT_MS = 1200;
  static const unsigned long PENDING_POST_TIMEOUT_MS = 30000;
}
