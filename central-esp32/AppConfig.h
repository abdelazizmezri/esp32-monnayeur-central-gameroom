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
  inline constexpr int DEFAULT_PIECES_REQUIRED = 1;
  // Valeur monétaire d'une impulsion validée, en millimes tunisiens.
  // 1000 millimes = 1 TND.
  inline constexpr int DEFAULT_PULSE_VALUE_MILLIMES = 1000;
  inline constexpr int DEFAULT_AVAILABLE_PIECES = 0;

  // Journal financier binaire compact dans la partition LittleFS.
  inline constexpr char REVENUE_FILE[] = "/revenue.bin";
  inline constexpr unsigned long REVENUE_STORAGE_RETRY_INTERVAL_MS = 30000;

  // Heure locale tunisienne (UTC+1, sans changement saisonnier actuel).
  inline constexpr char TIMEZONE[] = "CET-1";
  inline constexpr char NTP_SERVER_1[] = "pool.ntp.org";
  inline constexpr char NTP_SERVER_2[] = "time.nist.gov";

  // Entrée active LOW en 3,3 V avec pull-up interne ; réglage NO conseillé.
  // Verrouillage anti-doublon équivalent au projet de référence : timer = 20 tours
  // de boucle x 4 chiffres x 4 ms d'affichage, soit environ 320 ms après une pièce.
  // Les fronts suivants sont ignorés pendant cet intervalle, sans attente bloquante.
  inline constexpr unsigned long COIN_DEBOUNCE_MS = 320;
  // Le projet de référence attend 5 s pendant son animation de démarrage.
  // Ignorer les transitions d'initialisation avant d'armer l'entrée au repos HIGH.
  inline constexpr unsigned long COIN_STARTUP_IGNORE_MS = 5000;

  // Délais réseau des postes, indépendants du comptage des impulsions.
  inline constexpr unsigned long POST_REFRESH_INTERVAL_MS = 5000;
  inline constexpr unsigned long POST_OFFLINE_TIMEOUT_MS = 10000;
  inline constexpr unsigned long POSTE_STATUS_TIMEOUT_MS = 1200;
  // Une commande peut arriver pendant que le poste attend la réponse à son
  // annonce HTTP. Ce délai doit donc être supérieur au timeout de l'annonce.
  inline constexpr unsigned long POSTE_COMMAND_TIMEOUT_MS = 4000;
  inline constexpr unsigned long PENDING_POST_TIMEOUT_MS = 30000;
}
