#pragma once

namespace PosteConfig {
  inline constexpr char WIFI_SSID[] = "YOUR_WIFI";
  inline constexpr char WIFI_PASSWORD[] = "YOUR_PASSWORD";
  inline constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

  inline constexpr char WIFI_SETUP_AP_SSID_PREFIX[] = "GAMEROOM-POSTE";
  inline constexpr char WIFI_SETUP_AP_PASSWORD[] = "";

  inline constexpr char CENTRAL_MDNS_HOSTNAME[] = "pstm";
  inline constexpr unsigned long ANNOUNCE_INTERVAL_MS = 5000;

  // Sauvegarde périodique du temps restant pour proposer une reprise après coupure.
  // Une valeur courte améliore la précision mais augmente les écritures en mémoire flash.
  inline constexpr unsigned long SESSION_CHECKPOINT_INTERVAL_MS = 60000;

  inline constexpr char DEFAULT_POST_ID[] = "";
  inline constexpr char DEFAULT_POST_NAME[] = "";
  // Doit correspondre à AppConfig::POSTE_COMMAND_TOKEN sur la centrale.
  inline constexpr char COMMAND_TOKEN[] = "gameroom-poste-token-change-me";

  inline constexpr int RELAY_PIN = 27;

  inline constexpr bool RELAY_ACTIVE_HIGH = true;

  inline constexpr uint16_t HTTP_PORT = 80;
}
