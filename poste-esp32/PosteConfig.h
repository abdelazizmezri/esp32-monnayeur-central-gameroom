#pragma once

namespace PosteConfig {
  static const char* WIFI_SSID = "YOUR_WIFI";
  static const char* WIFI_PASSWORD = "YOUR_PASSWORD";
  static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

  static const char* WIFI_SETUP_AP_SSID_PREFIX = "GAMEROOM-POSTE-SETUP";
  static const char* WIFI_SETUP_AP_PASSWORD = "";
  static const unsigned long WIFI_SETUP_AP_SHUTDOWN_DELAY_MS = 5000;

  static const char* CENTRAL_MDNS_HOSTNAME = "gameroom";
  static const unsigned long ANNOUNCE_INTERVAL_MS = 5000;

  static const char* DEFAULT_POST_ID = "";
  static const char* DEFAULT_POST_NAME = "";
  // Must match AppConfig::POSTE_COMMAND_TOKEN on the central ESP32.
  static const char* COMMAND_TOKEN = "gameroom-poste-token-change-me";

  static const int RELAY_PIN = LED_BUILTIN;//26;

  static const bool RELAY_ACTIVE_HIGH = true;

  static const uint16_t HTTP_PORT = 80;
}
