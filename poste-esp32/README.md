# Poste — PS TIME MANAGER

Firmware d'un poste de jeu piloté par la centrale **PS TIME MANAGER**. Chaque
module de poste se connecte au réseau local, se fait découvrir par
la centrale, reçoit des commandes de session par HTTP et active un relais pendant
le temps de jeu attribué.

## Guides PDF

- [Guide technique client](../output/pdf/ps-time-manager-guide-technique-client.pdf)
  pour le câblage du relais et du TM1637, l'installation des outils, le
  téléversement et les essais ;
- [Manuel d'utilisation](../output/pdf/ps-time-manager-manuel-utilisation.pdf)
  pour la mise en réseau d'un poste et son exploitation depuis la centrale.

Les réseaux Wi-Fi, postes et états visibles dans les captures sont des données
de démonstration, sans information extraite d'une installation réelle.

## Sommaire

- [Guides PDF](#guides-pdf)
- [Fonctions principales](#fonctions-principales)
- [Place du poste dans le système](#place-du-poste-dans-le-système)
- [Fonctionnement du firmware](#fonctionnement-du-firmware)
- [Configuration](#configuration)
- [Affichage du temps restant](#affichage-du-temps-restant)
- [Câblage du poste](#câblage-du-poste)
- [Installation et compilation](#installation-et-compilation)
- [Première mise en service](#première-mise-en-service)
- [API HTTP du poste](#api-http-du-poste)
- [Annonce envoyée à la centrale](#annonce-envoyée-à-la-centrale)
- [Persistance des données](#persistance-des-données)
- [Rôle de chaque fichier](#rôle-de-chaque-fichier)
- [Sécurité](#sécurité)
- [Dépannage](#dépannage)
- [Limites connues](#limites-connues)

## Fonctions principales

- connexion à un Wi-Fi enregistré ou défini dans le code ;
- portail captif de configuration si la connexion Wi-Fi échoue ;
- identité persistante composée d'un identifiant et d'un nom ;
- découverte automatique par annonce HTTP auprès de `pstm.local` ;
- serveur HTTP local sur le port 80 ;
- démarrage, prolongation et arrêt d'une session ;
- activation d'un relais pendant toute la durée de la session ;
- arrêt automatique précis du relais dans une tâche FreeRTOS dédiée ;
- affichage local du temps restant sur quatre chiffres TM1637 ;
- remontée de l'état, de l'adresse IP et du temps restant à la centrale ;
- sauvegarde périodique du temps et reprise contrôlée après une coupure ;
- protection des commandes par un token Bearer partagé.

## Place du poste dans le système

```mermaid
flowchart LR
    Admin["Navigateur de l'administrateur"] -->|HTTP| Central["Centrale ESP32<br>pstm.local"]
    Central -->|"START_SESSION, ADD_TIME,<br>STOP_SESSION, configuration"| Poste["ESP32 du poste"]
    Poste -->|"Annonce périodique<br>et transitions de session"| Central
    Poste -->|GPIO| Relay["Module relais"]
    Poste -->|"GPIO 25 / 26"| Display["Afficheur TM1637"]
    Relay --> Machine["Machine ou poste de jeu"]
    Poste --> NVS["Mémoire NVS<br>Wi-Fi, identité et session"]
```

La centrale décide du temps à attribuer. Le poste ne gère ni les pièces ni les
crédits : il exécute seulement la durée reçue, commande la sortie du relais et
publie son état.

Les deux ESP32 doivent :

1. être connectés au même réseau local ;
2. utiliser le même token de communication ;
3. pouvoir communiquer directement en HTTP ;
4. disposer de mDNS afin que le poste puisse résoudre `pstm.local`.

## Fonctionnement du firmware

### Démarrage

La fonction `setup()` exécute les opérations suivantes :

1. ouvre le port série à 115200 bauds ;
2. construit l'identifiant matériel `chipId` depuis l'adresse eFuse de l'ESP32 ;
3. charge l'identifiant et le nom du poste depuis la NVS ;
4. configure la broche du relais et force le relais à l'arrêt ;
5. charge le dernier instantané de session et passe à `recovery_pending` s'il
   détecte une session interrompue, sinon à `idle` ;
6. initialise l'afficheur TM1637 et présente le temps correspondant à l'état ;
7. crée la tâche FreeRTOS `poste-control`, prioritaire sur la boucle Arduino ;
8. enregistre les routes du serveur HTTP ;
9. charge les identifiants Wi-Fi et tente une connexion pendant 15 secondes ;
10. ouvre le portail de configuration si la connexion échoue ;
11. démarre le serveur HTTP sur le port 80.

### Tâche de contrôle et boucle réseau

La tâche `poste-control` se réveille toutes les 20 ms et traite exclusivement :

- l'expiration de la session et la coupure du relais ;
- la sauvegarde périodique du temps restant ;
- la mise à jour du TM1637 à partir d'un instantané cohérent.

La fonction Arduino `loop()` reste consacrée au réseau :

- les requêtes HTTP entrantes ;
- le DNS du portail captif et l'état de la connexion Wi-Fi ;
- l'annonce périodique du poste à la centrale.

L'état de session est privé à `RelayService` et protégé par un mutex FreeRTOS.
Les routes HTTP, l'annonce et l'afficheur utilisent des instantanés atomiques :
ils ne peuvent donc jamais lire un mélange d'ancien et de nouvel état. Une
résolution mDNS ou une requête HTTP lente peut encore occuper la boucle réseau,
mais ne peut plus figer le décompte ni retarder la coupure du relais. Si la tâche
ne peut exceptionnellement pas être créée, un mode de secours reprend le
contrôle dans `loop()` et écrit un avertissement sur le port série.

### Cycle d'une session

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Active: START_SESSION(durée)
    Idle --> Active: ADD_TIME(durée)
    Active --> Active: ADD_TIME(durée)
    Active --> Idle: STOP_SESSION
    Active --> Idle: Temps écoulé
    Active --> RecoveryPending: Coupure puis redémarrage
    RecoveryPending --> Active: START_SESSION(temps sauvegardé)
    RecoveryPending --> Idle: STOP_SESSION / annulation
```

- `START_SESSION` remplace la date de fin par `maintenant + durée` ;
- `ADD_TIME` ajoute la durée à la session active, ou démarre une session si le
  relais est arrêté ;
- `STOP_SESSION` annule le temps restant et désactive immédiatement le relais ;
- le temps restant exposé par l'API est arrondi à la seconde supérieure.

### Reprise après une coupure électrique

Le poste écrit le temps restant dans le namespace NVS `session` au démarrage
d'une session, après chaque `ADD_TIME`, puis toutes les 60 secondes. Si
l'alimentation est coupée alors que cette valeur est positive, le démarrage suivant :

1. maintient obligatoirement le relais à l'arrêt ;
2. place le poste dans l'état `recovery_pending` ;
3. expose `recoveryPending=true` et `recoveryRemaining` dans `/status` ;
4. envoie les mêmes données dans chaque annonce à la centrale ;
5. attend `START_SESSION` pour reprendre ou `STOP_SESSION` pour annuler.

La centrale additionne éventuellement les minutes saisies par l'opérateur au
temps sauvegardé avant d'envoyer `START_SESSION`. Chaque poste est autonome : une
coupure touchant plusieurs postes produit plusieurs reprises indépendantes.

Cette sauvegarde ne mesure pas la durée de la coupure. Sans horloge RTC secourue,
le temps hors tension n'est pas déduit. De plus, un instantané vieux de moins de
60 secondes peut rendre jusqu'à environ une minute de plus que le temps affiché
juste avant la coupure. L'intervalle constitue un compromis entre précision et
usure de la mémoire flash.

### Configuration Wi-Fi et portail captif

Au démarrage, le firmware utilise en priorité les identifiants enregistrés dans
le namespace NVS `wifi`. S'ils n'existent pas, il essaie les valeurs
`WIFI_SSID` et `WIFI_PASSWORD` de `PosteConfig.h`, à condition que le SSID ne soit
plus `YOUR_WIFI`.

Si la connexion échoue, l'ESP32 crée un point d'accès :

| Paramètre | Valeur par défaut |
|---|---|
| Préfixe du SSID | `GAMEROOM-POSTE` |
| SSID réel | `GAMEROOM-POSTE-<chipId>` |
| Mot de passe | aucun, réseau ouvert |
| Adresse habituelle | `http://192.168.4.1` |

Le portail scanne les réseaux disponibles et permet aussi de saisir un SSID
manuellement. L'adressage est **automatique (DHCP)** par défaut. Le mode
**manuel (IP fixe)** demande une adresse IPv4, une passerelle et un masque ; les
DNS principal et secondaire sont facultatifs. Les identifiants et les paramètres
réseau ne sont enregistrés qu'après une connexion réussie. Le poste affiche
uniquement sa nouvelle adresse IP, puis redémarre automatiquement après huit
secondes.

L'ESP32 ne fournit pas de proxy HTTP système. Les annonces mDNS et les échanges
avec la centrale utilisent donc toujours une connexion directe au réseau local.
Le `chipId` provient de l'eFuse et est disponible même sur un poste neuf : chaque
poste diffuse ainsi un nom de point d'accès distinct avant son identification.

Quand le portail est actif, `/` et les URL usuelles de détection de portail
captif ouvrent la même page de configuration Wi-Fi que la centrale.

### Identité et découverte

L'identité du poste contient :

- `chipId` : identifiant matériel calculé automatiquement et non modifiable ;
- `id` : identifiant technique unique, dérivé automatiquement du `chipId` par la
  centrale et jamais demandé dans l'interface ;
- `name` : nom lisible, par exemple `Poste 1` ;
- `configured` : vrai uniquement si `id` et `name` ne sont pas vides.

Une fois connecté au Wi-Fi et hors du portail captif, le poste :

1. démarre mDNS sous le nom `poste-<chipId>` en minuscules ;
2. résout le nom `pstm.local` une première fois et mémorise son adresse IP ;
3. envoie son annonce à `POST /poste/announce` toutes les cinq secondes, ainsi
   qu'après une transition de session ;
4. invalide l'adresse mémorisée et relance mDNS uniquement après une erreur de
   transport HTTP.

Un poste sans identité apparaît dans les postes découverts de la centrale. La
centrale lui attribue ensuite automatiquement son `id` et transmet le `name`
saisi avec `POST /configure`.

## Configuration

Les constantes se trouvent dans `PosteConfig.h` :

| Constante | Valeur par défaut | Rôle |
|---|---|---|
| `WIFI_SSID` | `YOUR_WIFI` | SSID de secours compilé dans le firmware. |
| `WIFI_PASSWORD` | `YOUR_PASSWORD` | Mot de passe du SSID de secours. |
| `WIFI_CONNECT_TIMEOUT_MS` | `15000` | Délai maximal de connexion Wi-Fi. |
| `WIFI_SETUP_AP_SSID_PREFIX` | `GAMEROOM-POSTE` | Préfixe suivi automatiquement du `chipId` matériel. |
| `WIFI_SETUP_AP_PASSWORD` | chaîne vide | Mot de passe du point d'accès ; huit caractères minimum pour le protéger. |
| `CENTRAL_MDNS_HOSTNAME` | `pstm` | Nom mDNS de la centrale, sans `.local`. |
| `ANNOUNCE_INTERVAL_MS` | `5000` | Période normale d'annonce à la centrale. |
| `ANNOUNCE_STATE_CHANGE_DELAY_MS` | `250` | Délai minimal avant l'annonce accélérée d'une transition de session. |
| `ANNOUNCE_RETRY_INTERVAL_MS` | `5000` | Attente avant une nouvelle tentative après un échec. |
| `CENTRAL_MDNS_QUERY_TIMEOUT_MS` | `750` | Délai maximal de résolution mDNS quand l'IP centrale n'est pas connue. |
| `ANNOUNCE_HTTP_CONNECT_TIMEOUT_MS` | `400` | Délai maximal d'ouverture TCP vers la centrale. |
| `ANNOUNCE_HTTP_RESPONSE_TIMEOUT_MS` | `1200` | Délai maximal de lecture de la réponse d'annonce. |
| `SESSION_CHECKPOINT_INTERVAL_MS` | `60000` | Période de sauvegarde du temps restant pendant une session. |
| `DEFAULT_POST_ID` | chaîne vide | Identifiant initial si la NVS est vide. |
| `DEFAULT_POST_NAME` | chaîne vide | Nom initial si la NVS est vide. |
| `COMMAND_TOKEN` | valeur de développement | Secret partagé avec la centrale. |
| `RELAY_PIN` | GPIO 27 | GPIO commandant le relais. |
| `RELAY_ACTIVE_HIGH` | `true` | Niveau logique qui active le relais. |
| `TM1637_CLK_PIN` | GPIO 25 | Horloge de l'afficheur TM1637. |
| `TM1637_DIO_PIN` | GPIO 26 | Données de l'afficheur TM1637. |
| `TM1637_BRIGHTNESS` | `5` | Luminosité de 0 à 7. |
| `DISPLAY_UPDATE_INTERVAL_MS` | `100` | Période de vérification du contenu de l'afficheur. |
| `CONTROL_TASK_INTERVAL_MS` | `20` | Période de contrôle du relais et de la session. |
| `HTTP_PORT` | `80` | Port du serveur HTTP local. |

Avant le déploiement, modifier au minimum `COMMAND_TOKEN`, vérifier les trois
GPIO du relais et de l'afficheur puis, si nécessaire, adapter
`RELAY_ACTIVE_HIGH`. Le token doit être strictement identique à
`AppConfig::POSTE_COMMAND_TOKEN` dans le firmware de la centrale. Les assertions
de compilation empêchent d'attribuer la même broche au relais, à `CLK` ou à
`DIO`, et refusent une luminosité hors de la plage 0 à 7.

## Affichage du temps restant

Le pilote TM1637 est inclus directement dans le firmware : aucune bibliothèque
supplémentaire n'est nécessaire. Le format dépend automatiquement de la durée :

| État ou durée | Affichage | Comportement |
|---|---|---|
| Poste inactif | `00:00` | Fixe. |
| Session de moins d'une heure | `MM:SS` | Le séparateur clignote. |
| Session à partir d'une heure | `HH:MM` | Les minutes sont arrondies à la minute supérieure et le séparateur clignote. |
| Reprise après coupure | temps sauvegardé | Les quatre chiffres clignotent tant que la reprise attend une décision. |
| Durée supérieure à 99 h 59 | `99:59` | Valeur plafonnée à la capacité des quatre chiffres ; la session continue normalement. |

L'affichage ne détermine jamais l'expiration d'une session : le relais reste
piloté par `RelayService` avec la précision de `millis()`. Ainsi, l'arrondi du
mode `HH:MM` et le plafond visuel ne modifient pas le temps réel. Le pilote est
appelé par `poste-control`, indépendamment des délais mDNS et HTTP.

## Câblage du poste

Le schéma complet de la partie poste regroupe l'ESP32, l'afficheur et le relais :

![Schéma de câblage du poste ESP32 avec afficheur TM1637 et relais](docs/schema-cablage-poste-tm1637.svg)

### Afficheur TM1637 à quatre broches

Respecter ce câblage avec les valeurs par défaut :

| ESP32 | TM1637 | Fonction |
|---|---|---|
| `3V3` | `VCC` | Alimentation 3,3 V. |
| `GND` | `GND` | Masse commune. |
| GPIO 25 | `CLK` | Horloge, constante `TM1637_CLK_PIN`. |
| GPIO 26 | `DIO` | Données, constante `TM1637_DIO_PIN`. |

> Alimenter le module TM1637 en **3,3 V**. De nombreux modules relient leurs
> résistances de rappel de `CLK` et `DIO` à `VCC` ; avec une alimentation 5 V,
> ces lignes pourraient donc appliquer 5 V à l'ESP32, qui n'est pas tolérant au
> 5 V. Si un module impose réellement 5 V, intercaler un convertisseur de niveau
> bidirectionnel adapté sur `CLK` et `DIO`.

### Relais

Par défaut, le relais est piloté par le GPIO 27. Pour utiliser une autre broche,
remplacer cette valeur par le GPIO relié à l'entrée du module relais :

```cpp
inline constexpr int RELAY_PIN = 27;
inline constexpr bool RELAY_ACTIVE_HIGH = true;
```

Câblage logique typique :

| ESP32 | Module relais |
|---|---|
| GPIO défini par `RELAY_PIN` | `IN` |
| `GND` | `GND` |
| alimentation adaptée au module | `VCC` |

> Ne jamais alimenter directement une charge secteur ou une charge importante
> depuis un GPIO. Utiliser un module relais compatible 3,3 V, une alimentation
> appropriée et une isolation adaptée. Le câblage de la partie puissance doit
> être réalisé selon les règles de sécurité applicables.

Certains modules sont actifs à l'état bas. Dans ce cas, utiliser
`RELAY_ACTIVE_HIGH = false`. Au démarrage et à l'arrêt d'une session, le firmware
écrit toujours le niveau correspondant à l'état désactivé.

Le relais et le TM1637 doivent partager la masse logique de l'ESP32. Le relais
représenté sur le schéma est alimenté par `5V/VIN`, mais cette alimentation doit
être adaptée au modèle réellement utilisé. Ses contacts `COM` et `NO` sont des
contacts secs séparés du signal `IN`.

## Installation et compilation

### Prérequis

| Catégorie | Éléments requis |
|---|---|
| Carte | ESP32 Dev Module avec 4 Mo de flash et câble USB de données. |
| Périphériques | TM1637 quatre broches, module relais compatible 3,3 V et alimentation adaptée. |
| Outils | Ordinateur, multimètre, tournevis, pince à dénuder/sertir et matériel de soudure isolé. |
| Logiciel | Arduino IDE 2.3.10 ou `arduino-cli` 1.5.1. |
| Cœur de cartes | **esp32 by Espressif Systems** 3.3.11, cible `esp32:esp32:esp32`. |
| Bibliothèque externe | **ArduinoJson** 7.4.3. |

`WiFi`, `WebServer`, `DNSServer`, `Preferences`, `ESPmDNS`, `HTTPClient` et
FreeRTOS sont fournis par le cœur ESP32 3.3.11. Le pilote TM1637 fait partie de
`DisplayService.cpp` : ne pas installer de bibliothèque TM1637 externe. La
compilation finale utilise 1 134 216 octets de programme et 51 632 octets de
variables globales. La partition applicative par défaut accepte 1 310 720
octets : il reste environ 176 504 octets pour les évolutions.

### Avec Arduino IDE

1. Ouvrir `poste-esp32.ino`.
2. Installer **ArduinoJson** depuis le gestionnaire de bibliothèques.
3. Sélectionner une carte compatible, par exemple **ESP32 Dev Module**.
4. Régler **Flash Size = 4MB (32Mb)** et conserver le schéma
   **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)**. Le schéma
   **Minimal SPIFFS** ne concerne que la centrale qui embarque les visuels HD.
5. Conserver **CPU Frequency = 240 MHz (WiFi/BT)**, **Flash Frequency = 80 MHz**,
   **Flash Mode = QIO**, **PSRAM = Disabled** et **Erase All Flash = Disabled**.
6. Adapter les constantes de `PosteConfig.h`, surtout `COMMAND_TOKEN`,
   `RELAY_PIN`, `RELAY_ACTIVE_HIGH`, `TM1637_CLK_PIN` et `TM1637_DIO_PIN`.
7. Pour le premier essai, débrancher la charge des contacts du relais.
8. Sélectionner le port série de la carte.
9. Cliquer sur **Vérifier**, puis **Téléverser**.
10. Ouvrir le moniteur série à 115200 bauds et vérifier `00:00` sur le TM1637.

### Avec arduino-cli

```bash
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.11
arduino-cli lib install ArduinoJson@7.4.3
arduino-cli board list
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=default .
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=default --port PORT_SERIE .
```

Adapter `PORT_SERIE`, par exemple `/dev/cu.usbserial-0001` sous macOS ou `COM3`
sous Windows.

## Première mise en service

1. Vérifier que la centrale fonctionne et publie `pstm.local`.
2. Téléverser le firmware du poste après avoir configuré le token et le relais.
3. Ouvrir le moniteur série à 115200 bauds.
4. Si aucun Wi-Fi valide n'est connu, se connecter au point d'accès
   `GAMEROOM-POSTE-<chipId>` affiché dans le moniteur série.
5. Ouvrir `http://192.168.4.1` si le portail ne s'affiche pas automatiquement.
6. Choisir le même réseau local que celui de la centrale et saisir son mot de
   passe.
7. Attendre le message de connexion et noter l'adresse IP affichée.
8. Dans l'interface de la centrale, ouvrir **Gestion des postes**.
9. Attendre l'apparition du nouveau `chipId` dans les postes découverts.
10. Attribuer uniquement un nom visible au poste. La centrale dérive
    automatiquement l'identifiant technique depuis le `chipId`.
11. Vérifier que l'afficheur indique `00:00` au repos.
12. Affecter du temps et vérifier le passage à `active`, l'activation du relais
    et le décompte du temps restant sur le TM1637.

## API HTTP du poste

Le serveur écoute en HTTP sur le port 80. Les routes de commande attendent cet
en-tête :

```http
Authorization: Bearer VOTRE_TOKEN_PARTAGE
```

Le firmware refuse toutes les routes protégées si `COMMAND_TOKEN` contient moins
de 16 caractères.

### Routes

| Méthode | Route | Authentification | Description |
|---|---|---|---|
| GET | `/` | Non | Retourne `Poste OK`, ou redirige vers le portail Wi-Fi. |
| GET | `/status` | Non | Retourne l'identité, l'état du relais et le temps restant. |
| POST | `/configure` | Bearer | Enregistre un `id` et un `name` non vides. |
| POST | `/identity/clear` | Bearer | Arrête la session et efface l'identité du poste. |
| POST | `/command` | Bearer | Exécute une commande de session ou un ping. |
| GET | `/wifi` | Non, portail uniquement | Affiche le formulaire de configuration Wi-Fi. |
| POST | `/wifi/save` | Non, portail uniquement | Teste et sauvegarde le Wi-Fi soumis par le formulaire. |

Une route protégée réussie renvoie :

```json
{"ok": true}
```

Une erreur renvoie un statut HTTP `400` ou `401` et un objet de la forme :

```json
{"error": "message"}
```

### Lire l'état

```bash
curl http://ADRESSE_IP_DU_POSTE/status
```

Exemple de réponse :

```json
{
  "chipId": "1234ABCDEF12",
  "configured": true,
  "id": "post1",
  "name": "Poste 1",
  "ip": "192.168.1.42",
  "status": "active",
  "relay": true,
  "remaining": 1784,
  "recoveryPending": false,
  "recoveryRemaining": 0
}
```

Après une coupure, `status` vaut `recovery_pending`, `relay` et `remaining`
valent respectivement `false` et `0`, tandis que `recoveryRemaining` contient le
dernier temps sauvegardé.

### Configurer l'identité

```bash
curl -X POST http://ADRESSE_IP_DU_POSTE/configure \
  -H 'Authorization: Bearer VOTRE_TOKEN_PARTAGE' \
  -H 'Content-Type: application/json' \
  -d '{"id":"post1","name":"Poste 1"}'
```

### Démarrer une session

La durée est exprimée en secondes :

```bash
curl -X POST http://ADRESSE_IP_DU_POSTE/command \
  -H 'Authorization: Bearer VOTRE_TOKEN_PARTAGE' \
  -H 'Content-Type: application/json' \
  -d '{"action":"START_SESSION","duration":1800}'
```

### Ajouter du temps

```bash
curl -X POST http://ADRESSE_IP_DU_POSTE/command \
  -H 'Authorization: Bearer VOTRE_TOKEN_PARTAGE' \
  -H 'Content-Type: application/json' \
  -d '{"action":"ADD_TIME","duration":900}'
```

### Arrêter une session

```bash
curl -X POST http://ADRESSE_IP_DU_POSTE/command \
  -H 'Authorization: Bearer VOTRE_TOKEN_PARTAGE' \
  -H 'Content-Type: application/json' \
  -d '{"action":"STOP_SESSION"}'
```

### Tester la communication

La commande `PING` ne modifie pas l'état ; une réponse `200` confirme seulement
que le poste accepte le token et traite les commandes.

```bash
curl -X POST http://ADRESSE_IP_DU_POSTE/command \
  -H 'Authorization: Bearer VOTRE_TOKEN_PARTAGE' \
  -H 'Content-Type: application/json' \
  -d '{"action":"PING"}'
```

## Annonce envoyée à la centrale

Le poste envoie une requête périodique toutes les cinq secondes et accélère
l'envoi après un démarrage, un ajout, un arrêt ou une expiration de session :

```text
POST http://pstm.local/poste/announce
```

L'en-tête Bearer et `Content-Type: application/json` sont ajoutés. Le contenu est
équivalent à :

```json
{
  "chipId": "1234ABCDEF12",
  "configured": true,
  "id": "post1",
  "name": "Poste 1",
  "status": "active",
  "relay": true,
  "remaining": 1784,
  "recoveryPending": false,
  "recoveryRemaining": 0,
  "ip": "192.168.1.42"
}
```

L'annonce n'est pas envoyée tant que le Wi-Fi est déconnecté ou que le portail
captif est actif. L'échec d'une annonce n'arrête pas le poste ni une session en
cours ; une nouvelle tentative a lieu après cinq secondes. Chaque changement de
session incrémente une révision interne. Si l'état change pendant un envoi HTTP,
la différence de révision provoque une nouvelle annonce après au moins 250 ms,
ce qui évite qu'un instantané devenu ancien reste visible sur la centrale.

La résolution `pstm.local` n'est plus répétée à chaque envoi. L'IP résolue est
réutilisée tant que le transport fonctionne ; cela réduit les appels bloquants
et laisse davantage de temps à la réception des commandes centrales.

## Persistance des données

Le firmware utilise `Preferences`, donc la mémoire NVS de l'ESP32 :

| Namespace | Clés | Données |
|---|---|---|
| `wifi` | `ssid`, `password` | Réseau Wi-Fi validé par le portail. |
| `identity` | `id`, `name` | Identité logique attribuée par la centrale. |
| `session` | `remaining` | Dernier temps restant sauvegardé ; une valeur positive indique une session interrompue. |

Le `chipId` est recalculé depuis l'eFuse à chaque démarrage.

La date de fin basée sur `millis()` et l'état instantané du relais restent en RAM.
Un redémarrage coupe donc toujours le relais. Si une session active avait été
sauvegardée, le poste conserve toutefois son temps récupérable et attend la
décision de reprise de la centrale au lieu de revenir directement à `idle`.

## Rôle de chaque fichier

| Fichier | Rôle |
|---|---|
| `poste-esp32.ino` | Point d'entrée Arduino. Initialise l'identité, la tâche de contrôle, le serveur, le Wi-Fi et les annonces. |
| `PosteConfig.h` | Centralise les constantes : Wi-Fi, mDNS, timeouts réseau, tâche de contrôle, sauvegarde, GPIO et port HTTP. |
| `PosteState.h` | Définit uniquement l'état réseau et l'identité partagés par la boucle HTTP. L'état de session est encapsulé dans `RelayService`. |
| `PosteControlService.h` | Déclare l'initialisation de la tâche temps réel et son mode de secours. |
| `PosteControlService.cpp` | Exécute relais et afficheur toutes les 20 ms dans une tâche FreeRTOS prioritaire. |
| `RelayService.h` | Définit les commandes de session et la structure d'instantané atomique. |
| `RelayService.cpp` | Protège l'état par mutex, pilote le relais, gère le temps et sauvegarde/restaure la session en NVS. |
| `DisplayService.h` | Déclare l'initialisation et la mise à jour de l'afficheur. |
| `DisplayService.cpp` | Pilote directement le bus TM1637, formate le temps et gère les états fixe/clignotant sans bibliothèque externe. |
| `PosteIdentityService.h` | Déclare le chargement, l'enregistrement et l'effacement de l'identité du poste. |
| `PosteIdentityService.cpp` | Construit le `chipId` depuis l'eFuse et conserve `id`/`name` dans le namespace NVS `identity`. |
| `PosteAnnounceService.h` | Déclare la mise à jour périodique de l'annonce. |
| `PosteAnnounceService.cpp` | Met en cache l'IP centrale, applique les timeouts HTTP et annonce périodiquement ou immédiatement les transitions. |
| `PosteWebServer.h` | Déclare l'enregistrement des routes HTTP du poste. |
| `PosteWebServer.cpp` | Implémente `/status`, `/configure`, `/identity/clear`, `/command`, les réponses JSON et la validation du token Bearer. |
| `WiFiConfigService.h` | Déclare l'initialisation et le suivi de la connexion Wi-Fi. |
| `WiFiConfigService.cpp` | Charge et sauvegarde les identifiants Wi-Fi, tente la connexion, crée le point d'accès, sert le portail captif et traite son DNS. |

Les fichiers `.h` exposent les interfaces et structures ; les fichiers `.cpp`
contiennent leur implémentation. Cette séparation permet au fichier `.ino` de
rester limité à l'orchestration générale.

## Sécurité

Avant une utilisation réelle :

1. remplacer `COMMAND_TOKEN` par un secret long et différent de la valeur du
   dépôt ;
2. reporter exactement le même secret dans `POSTE_COMMAND_TOKEN` sur la
   centrale ;
3. protéger le point d'accès de configuration avec un mot de passe d'au moins
   huit caractères ;
4. utiliser un réseau local privé et isolé ;
5. ne pas rediriger le port 80 du poste vers Internet ;
6. protéger l'accès physique à l'ESP32 et à son câblage.

Les communications utilisent HTTP sans TLS. Le statut est public sur le réseau
local. Le token et les éventuels identifiants Wi-Fi compilés sont présents dans
le firmware, tandis que les identifiants configurés par le portail sont stockés
dans la NVS sans chiffrement applicatif.

## Dépannage

### Le portail Wi-Fi ne s'ouvre pas

- vérifier la connexion au SSID `GAMEROOM-POSTE-<chipId>` ;
- ouvrir manuellement `http://192.168.4.1` ;
- désactiver temporairement les données mobiles ou le VPN ;
- consulter le moniteur série à 115200 bauds ;
- redémarrer le poste pour relancer la tentative de connexion et le portail.

### Le poste n'apparaît pas dans la centrale

- vérifier que les deux ESP32 sont sur le même réseau ;
- ouvrir `http://ADRESSE_IP_DU_POSTE/status` ;
- vérifier que `pstm.local` résout vers la centrale ;
- comparer `COMMAND_TOKEN` du poste et `POSTE_COMMAND_TOKEN` de la centrale ;
- vérifier que le réseau n'isole pas ses clients Wi-Fi ;
- attendre au moins cinq secondes après la connexion.

### Les commandes renvoient `401 unauthorized`

- envoyer l'en-tête `Authorization: Bearer ...` ;
- utiliser exactement le token de `PosteConfig.h` ;
- vérifier que le token contient au moins 16 caractères ;
- recompiler et téléverser le poste après toute modification du token.

### Le relais fonctionne à l'envers

Inverser `RELAY_ACTIVE_HIGH`. Utiliser `true` si l'entrée du relais s'active au
niveau haut et `false` si elle s'active au niveau bas.

### Le relais ne commute pas

- vérifier que `RELAY_PIN` correspond au GPIO réellement câblé ;
- vérifier la masse commune et l'alimentation du module ;
- confirmer que le module accepte un signal logique de 3,3 V ;
- envoyer manuellement `START_SESSION`, puis consulter `/status` ;
- observer les messages `START_SESSION` et `STOP_SESSION` sur le port série.

### L'afficheur TM1637 reste éteint ou incohérent

- vérifier `3V3 → VCC` et la masse commune ;
- vérifier `GPIO 25 → CLK` et `GPIO 26 → DIO`, certains modules présentant les
  broches dans un ordre différent ;
- confirmer les valeurs `TM1637_CLK_PIN` et `TM1637_DIO_PIN` dans
  `PosteConfig.h` puis recompiler ;
- augmenter temporairement `TM1637_BRIGHTNESS` jusqu'à 7 ;
- ne pas alimenter le module en 5 V sans convertisseur de niveau sur les deux
  lignes logiques ;
- tester d'abord sans la charge de puissance raccordée au relais afin d'écarter
  les perturbations d'alimentation.

### Le décompte se fige encore pendant les échanges réseau

- vérifier dans le moniteur série la présence de `Poste control task started.` ;
- si `Control task allocation failed` apparaît, redémarrer le poste et vérifier
  qu'aucune extension du firmware ne consomme anormalement la RAM ;
- contrôler que `CONTROL_TASK_INTERVAL_MS` reste à une valeur raisonnable, 20 ms
  par défaut ;
- distinguer un gel réel du mode `HH:MM`, qui ne change volontairement qu'à la
  minute lorsque la durée restante atteint une heure.

## Limites connues

- le relais repart toujours désactivé et une session interrompue exige une
  relance ou une annulation manuelle depuis la centrale ;
- le dernier instantané peut restituer environ 60 secondes de plus que le temps
  réel et ne déduit pas le temps passé hors tension ;
- diminuer fortement `SESSION_CHECKPOINT_INTERVAL_MS` augmente les écritures et
  donc l'usure de la mémoire flash ;
- les communications et le portail utilisent HTTP sans chiffrement ;
- le portail Wi-Fi est ouvert avec la configuration par défaut ;
- il n'existe pas de route HTTP dédiée à l'effacement des identifiants Wi-Fi ;
- le firmware n'impose pas de durée maximale de session ;
- l'affichage est plafonné à `99:59`, sans plafonner la durée réelle de la session ;
- la résolution de `pstm.local` dépend du support mDNS du réseau ;
- en cas d'échec exceptionnel de création de la tâche FreeRTOS, le mode de
  secours maintient les fonctions mais le réseau peut de nouveau retarder leur
  actualisation jusqu'au prochain redémarrage ;
- le serveur HTTP Arduino traite un nombre limité de requêtes et convient à un
  réseau local, pas à une exposition publique ;
- le changement de `COMMAND_TOKEN` nécessite de recompiler chaque poste et de
  garder la même valeur sur la centrale.
