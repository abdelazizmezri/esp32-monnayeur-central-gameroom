#pragma once

namespace PosteConfig {
  static const char* WIFI_SSID = "YOUR_WIFI";
  static const char* WIFI_PASSWORD = "YOUR_PASSWORD";
  static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

  static const char* WIFI_SETUP_AP_SSID_PREFIX = "ESP32-SETUP-";
  static const char* WIFI_SETUP_AP_PASSWORD = "";
  static const unsigned long WIFI_SETUP_AP_SHUTDOWN_DELAY_MS = 5000;

  static const char* POST_ID = "post1";
  static const char* POST_NAME = "Poste 1";

  static const int RELAY_PIN = 26;

  static const bool RELAY_ACTIVE_HIGH = true;

  static const uint16_t HTTP_PORT = 80;
}
