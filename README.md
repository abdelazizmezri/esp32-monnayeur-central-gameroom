# PS TIME MANAGER sur ESP32

Ce dépôt contient les deux firmwares Arduino qui composent un système local de
gestion de postes de jeu à monnayeur :

- **`central-esp32`** : la centrale reçoit les impulsions du monnayeur, conserve
  le solde de coins et distribue du temps de jeu depuis une interface web ;
- **`poste-esp32`** : le contrôleur installé sur chaque poste reçoit la durée de
  jeu, commande un relais et renvoie son état à la centrale.

Chaque dossier est un sketch Arduino autonome à compiler et à téléverser sur un
ESP32 différent. Une centrale peut piloter plusieurs postes qui exécutent tous
le même firmware, avec une identité propre enregistrée en mémoire.

## Documentation

- [Manuel complet de la centrale](central-esp32/README.md)
- [Manuel complet d'un poste](poste-esp32/README.md)

Ces deux manuels détaillent la mise en service, toutes les routes HTTP, les
exemples `curl`, la persistance, le dépannage et les limites propres à chaque
firmware. Le présent README décrit surtout l'architecture et le fonctionnement
du système complet.

## Architecture générale

```mermaid
flowchart LR
    M["Monnayeur"] -->|"Impulsions électriques"| C["ESP32 centrale"]
    U["Navigateur local"] <-->|"Interface web et API HTTP"| C
    C <-->|"Commandes, annonces et états HTTP"| P1["ESP32 poste 1"]
    C <-->|"Commandes, annonces et états HTTP"| P2["ESP32 poste 2"]
    C <-->|"Commandes, annonces et états HTTP"| PN["ESP32 poste N"]
    P1 --> R1["Relais poste 1"]
    P2 --> R2["Relais poste 2"]
    PN --> RN["Relais poste N"]
    C --> NC["NVS centrale"]
    P1 --> NP["NVS du poste"]
```

La centrale est le point de décision. Un poste ne compte pas les pièces et ne
gère pas le solde : il exécute uniquement les commandes de session reçues et
maintient son relais actif pendant la durée demandée.

## Fonctionnement du système

### 1. Démarrage de la centrale

La centrale charge depuis sa mémoire NVS :

- la configuration des coins ;
- le solde disponible ;
- les comptes et le token de l'API ;
- la liste des postes connus ;
- les identifiants Wi-Fi.

Elle crée ou migre ensuite le compte administrateur, se connecte au Wi-Fi et
publie `http://pstm.local` avec mDNS. Si aucun réseau valide n'est enregistré,
elle ouvre un point d'accès `GAMEROOM-CENTRAL-<chipId>` et son portail captif.

### 2. Démarrage d'un poste

Le poste calcule son `chipId` matériel, charge son identité et ses identifiants
Wi-Fi depuis la NVS, puis force le relais à l'arrêt. En cas d'échec Wi-Fi, il
ouvre un point d'accès `GAMEROOM-POSTE-<chipId>`.

Une fois connecté, il publie son propre service mDNS, résout `pstm.local` et
annonce son état à la centrale toutes les cinq secondes.

### 3. Découverte et configuration

Un poste neuf n'a pas encore de nom. Son annonce le place dans la liste des
postes découverts de la centrale. Depuis l'interface d'administration, on lui
attribue uniquement un nom visible, par exemple `Poste 1`. La centrale utilise
automatiquement le `chipId` matériel comme identifiant technique unique et ne le
demande pas à l'utilisateur.

La centrale transmet cette identité à `POST /configure`. Les deux appareils la
sauvegardent ensuite dans leur NVS. Une annonce absente pendant 30 secondes fait
disparaître un poste non encore configuré de la liste d'attente.

### 4. Réception d'une pièce

Le monnayeur est lu sur le GPIO 13 de la centrale, en entrée simple, sur chaque
changement d'état. Le firmware détecte automatiquement la polarité au repos
(contact NO ou NC), applique un anti-rebond de 80 ms et ne compte que les
impulsions complètes. Le nombre d'impulsions requis par coin est configurable ;
chaque groupe complet augmente le solde central persistant.

Valeurs par défaut :

| Paramètre | Valeur |
|---|---:|
| Impulsions par coin | 1 |
| Durée par coin | 1 800 s, soit 30 min |
| Solde initial | 0 coin |

### 5. Affectation du temps à un poste

Lorsqu'un utilisateur affecte des coins, la centrale calcule :

```text
durée envoyée = coins affectés × durée d'un coin
```

Elle envoie ensuite au poste :

- `START_SESSION` si le poste est inactif ;
- `ADD_TIME` s'il est déjà actif ;
- `STOP_SESSION` lors d'un arrêt manuel.

Les coins ne sont débités qu'après une réponse HTTP réussie du poste. Sur le
poste, `START_SESSION` active le relais et fixe la date de fin ; `ADD_TIME`
prolonge cette date. La boucle principale coupe automatiquement le relais quand
le temps arrive à zéro.

### 6. Supervision

En plus des annonces, la centrale interroge un poste configuré toutes les cinq
secondes, à tour de rôle, avec `GET /status`. Elle affiche son état, son IP, le
relais et le temps restant. Un poste sans réponse valide depuis plus de dix
secondes passe à l'état `offline`.

### 7. Reprise après une coupure électrique

Pendant une session, chaque poste enregistre un instantané du temps restant dans
sa NVS au démarrage de la session, après chaque ajout de temps, puis toutes les
60 secondes. Si le poste redémarre alors qu'un instantané actif existe :

1. le relais reste coupé pour éviter une reprise non contrôlée ;
2. le poste passe à l'état `recovery_pending` ;
3. il annonce séparément le temps récupérable à la centrale ;
4. la centrale affiche **Reprise en attente** sur la carte du poste concerné ;
5. l'opérateur peut **Reprendre** le temps sauvegardé ou l'**Annuler**.

Chaque poste possède son propre état de reprise. Plusieurs sessions interrompues
peuvent donc être traitées indépendamment depuis la centrale. Une relance envoie
`START_SESSION` avec le temps sauvegardé. Une annulation envoie `STOP_SESSION`
et efface définitivement l'instantané du poste.

L'ESP32 ne disposant pas ici d'une horloge secourue, le temps passé hors tension
n'est pas déduit. Avec l'intervalle par défaut, le temps proposé peut également
être supérieur au temps réel d'au plus environ 60 secondes.

## Communication entre les deux firmwares

Les ESP32 doivent être sur un réseau local qui autorise les communications entre
clients et la résolution mDNS.

| Sens | Route | Rôle |
|---|---|---|
| Poste vers centrale | `POST /poste/announce` | Annonce l'identité, l'IP, l'état, le relais et le temps restant. |
| Centrale vers poste | `GET /status` | Lit l'état courant du poste. |
| Centrale vers poste | `POST /configure` | Attribue ou modifie l'identité logique. |
| Centrale vers poste | `POST /identity/clear` | Efface l'identité et remet le poste en découverte. |
| Centrale vers poste | `POST /command` | Envoie `START_SESSION`, `ADD_TIME`, `STOP_SESSION` ou `PING`. |

La centrale expose également `POST /recovery/resume` et
`POST /recovery/cancel` aux comptes connectés et aux clients possédant son token
API. Ces routes règlent individuellement la reprise de chaque poste.

Les annonces et commandes utilisent un secret Bearer partagé. Ces deux
constantes doivent contenir exactement la même valeur :

```cpp
// central-esp32/AppConfig.h
POSTE_COMMAND_TOKEN

// poste-esp32/PosteConfig.h
COMMAND_TOKEN
```

Ce secret inter-ESP32 est distinct du token API généré par la centrale pour les
clients externes.

## Interface et droits

La centrale fournit une interface web responsive sur son IP et sur
`http://pstm.local`.

| Fonction | Administrateur | Utilisateur simple |
|---|:---:|:---:|
| Voir les postes et le solde | Oui | Oui |
| Affecter des coins et arrêter une session | Oui | Oui |
| Relancer ou annuler une session interrompue | Oui | Oui |
| Simuler une pièce | Oui | Non |
| Configurer ou supprimer les postes | Oui | Non |
| Modifier la configuration et le Wi-Fi | Oui | Non |
| Gérer les comptes, les logs et le token API | Oui | Non |

Dans **Vue des postes**, le bouton **Masquer les cartes** / **Afficher les cartes**
permet de replier collectivement les cinq indicateurs supérieurs (coins, durée,
postes actifs, postes hors ligne et reprises). Le choix est mémorisé dans le
navigateur et ne suspend pas l'actualisation des données masquées.

Au premier démarrage, le compte est `admin` avec le mot de passe `12345678`.
Il doit être modifié immédiatement. Les mots de passe sont salés et hachés en
SHA-256 avant leur stockage ; les sessions web restent uniquement en RAM.

La page **Configuration** permet aussi d'exporter et de restaurer une sauvegarde
JSON complète : paramètres de coins, solde, token API, utilisateurs avec leurs
identifiants chiffrés, postes configurés, réglages Wi-Fi/IP et préférence
d'affichage des cartes d'indicateurs. Une restauration ferme les sessions web,
applique cette préférence dans le navigateur courant et demande une nouvelle
connexion. Le fichier contient des données sensibles et doit être protégé.

## Matériel et câblage

Matériel minimal :

- un ESP32 pour la centrale ;
- un ESP32 par poste de jeu ;
- un monnayeur à impulsions avec une adaptation électrique vers 3,3 V ;
- un module relais adapté à chaque charge ;
- un réseau Wi-Fi local ;
- des alimentations adaptées et un câblage sécurisé.

La centrale utilise par défaut le GPIO 13 pour le monnayeur. Le firmware du
poste utilise le GPIO 27 pour le relais ; avant de connecter une machine, il
faut vérifier `RELAY_PIN` dans `PosteConfig.h` et adapter `RELAY_ACTIVE_HIGH` au
module relais.

> Un GPIO ESP32 n'accepte pas directement les tensions d'un monnayeur et ne doit
> jamais alimenter une charge de puissance. Utiliser une interface 3,3 V,
> l'isolation et les protections adaptées. Le câblage secteur doit être confié à
> une personne qualifiée.

## Installation de l'environnement et téléversement

Cette procédure part d'une installation neuve d'Arduino IDE 2 et va jusqu'au
premier démarrage des deux firmwares. Le projet a été vérifié avec le cœur ESP32
3.3.11 et ArduinoJson 7.4.3.

### 1. Installer Arduino IDE

1. Télécharger **Arduino IDE 2** depuis la
   [page officielle Arduino](https://www.arduino.cc/en/software).
2. Installer puis démarrer l'application.
3. Sous macOS, autoriser l'application si le système le demande. Sous Windows,
   accepter l'installation du pilote série proposé avec la carte.

Voir également le
[guide officiel d'installation d'Arduino IDE](https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE).

### 2. Ajouter les cartes ESP32 à Arduino IDE

1. Ouvrir **Arduino IDE → Settings** sur macOS ou
   **File → Preferences** sous Windows/Linux.
2. Dans **Additional Boards Manager URLs**, ajouter l'adresse stable officielle :

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

   Si une autre adresse est déjà présente, ajouter celle-ci sur une nouvelle
   ligne ou la séparer par une virgule.
3. Ouvrir **Tools → Board → Boards Manager**.
4. Rechercher `esp32` puis installer **esp32 by Espressif Systems**.
5. Redémarrer Arduino IDE après l'installation.

La procédure et l'URL sont publiées dans la
[documentation officielle Arduino-ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

### 3. Installer les bibliothèques nécessaires

La seule bibliothèque à installer séparément est :

- **ArduinoJson**, par Benoit Blanchon — version 7.4.3 testée.

Dans Arduino IDE :

1. Ouvrir **Tools → Manage Libraries** ou l'icône **Library Manager** dans la
   barre latérale.
2. Rechercher `ArduinoJson`.
3. Sélectionner la bibliothèque de **Benoit Blanchon**, puis cliquer sur
   **Install**.

`WiFi`, `WebServer`, `DNSServer`, `Preferences`, `ESPmDNS`, `HTTPClient` et
mbedTLS sont installés automatiquement avec **esp32 by Espressif Systems**. Il
ne faut pas installer d'autres bibliothèques portant ces noms. Voir le
[guide officiel du gestionnaire de bibliothèques](https://support.arduino.cc/hc/en-us/articles/5145457742236-Add-libraries-to-Arduino-IDE).

### 4. Brancher et sélectionner l'ESP32

1. Utiliser un câble USB qui transporte les **données** et pas uniquement
   l'alimentation.
2. Pour le premier essai, débrancher le monnayeur, le relais et toute charge de
   puissance de la carte.
3. Brancher l'ESP32 au PC.
4. Sélectionner **Tools → Board → ESP32 Arduino → ESP32 Dev Module**, ou le
   modèle exact de la carte s'il est connu.
5. Sélectionner le port dans **Tools → Port**.

Si aucun port n'apparaît, essayer un autre câble ou port USB, puis installer le
pilote correspondant au convertisseur USB-série de la carte, souvent CP210x ou
CH340. La sélection correcte de la carte est détaillée dans le
[guide Espressif du menu Tools](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html).

### 5. Régler la carte

Utiliser les réglages suivants pour une carte ESP32 classique avec 4 Mo de
flash :

| Réglage Arduino IDE | Centrale | Poste |
|---|---|---|
| Board | ESP32 Dev Module | ESP32 Dev Module |
| Flash Size | 4MB | 4MB |
| Partition Scheme | Default 4MB with spiffs | Default 4MB with spiffs |
| Upload Speed | 460800, ou 115200 en cas d'échec | 460800, ou 115200 en cas d'échec |
| Port | Port USB de la centrale | Port USB du poste concerné |

Le projet utilise uniquement le partitionnement standard fourni avec la carte.
Aucun fichier de partitions personnalisé n'est nécessaire. Les images intégrées
au firmware sont allégées afin que la centrale reste compatible avec la
partition d'application standard.

### 6. Préparer la configuration du projet

Avant de compiler :

1. Dans [`central-esp32/AppConfig.h`](central-esp32/AppConfig.h), remplacer
   `POSTE_COMMAND_TOKEN` par un secret long et vérifier `COIN_PIN`, actuellement
   réglé sur le GPIO 13.
2. Dans [`poste-esp32/PosteConfig.h`](poste-esp32/PosteConfig.h), copier
   exactement le même secret dans `COMMAND_TOKEN`.
3. Vérifier `RELAY_PIN`, actuellement GPIO 27, et adapter
   `RELAY_ACTIVE_HIGH` au module relais utilisé.

Les identifiants du réseau Wi-Fi ne sont pas écrits dans le code : ils seront
saisis plus tard dans les portails de configuration du central et des postes.

### 7. Compiler et téléverser la centrale

1. Ouvrir [`central-esp32/central-esp32.ino`](central-esp32/central-esp32.ino).
2. Vérifier de nouveau la carte, le port et le schéma de partitions par défaut.
3. Cliquer sur **Verify** pour compiler.
4. Cliquer sur **Upload** pour téléverser le firmware.
5. Si Arduino reste sur `Connecting...`, maintenir le bouton **BOOT** de
   l'ESP32, lancer le téléversement, puis relâcher BOOT dès que l'écriture
   commence. Appuyer ensuite brièvement sur **EN/RESET** si nécessaire.
6. Ouvrir **Tools → Serial Monitor** et choisir **115200 baud**.

La centrale démarre soit sur le réseau enregistré, soit avec le point d'accès
`GAMEROOM-CENTRAL-<chipId>`.

### 8. Compiler et téléverser un poste

1. Ouvrir [`poste-esp32/poste-esp32.ino`](poste-esp32/poste-esp32.ino) dans une
   autre fenêtre Arduino IDE.
2. Sélectionner l'ESP32 du poste, son port USB et le schéma de partitions par
   défaut.
3. Cliquer sur **Verify**, puis **Upload**.
4. Ouvrir le moniteur série à **115200 baud**.
5. Répéter l'opération sur chaque poste. Tous utilisent le même firmware et le
   même secret, mais chaque ESP32 possède un `chipId` matériel unique.

### 9. Installation équivalente avec arduino-cli

```bash
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install ArduinoJson
arduino-cli board list
```

Compiler et téléverser la centrale :

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 central-esp32
arduino-cli upload --fqbn esp32:esp32:esp32 --port PORT_CENTRALE central-esp32
```

Compiler et téléverser un poste :

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 poste-esp32
arduino-cli upload --fqbn esp32:esp32:esp32 --port PORT_POSTE poste-esp32
```

Remplacer `PORT_CENTRALE` et `PORT_POSTE` par les valeurs retournées par
`arduino-cli board list`, par exemple `COM5` sous Windows ou
`/dev/cu.usbserial-...` sous macOS.

## Mise en service rapide

1. Téléverser et démarrer la centrale.
2. Se connecter au réseau ouvert `GAMEROOM-CENTRAL-<chipId>` (sans mot de passe).
3. Ouvrir `http://192.168.4.1` et sélectionner le réseau local.
4. Ouvrir `http://pstm.local`, se connecter avec `admin` / `12345678`, puis
   changer immédiatement le mot de passe.
5. Téléverser et démarrer un poste configuré avec le même secret partagé.
6. Si nécessaire, rejoindre son point d'accès `GAMEROOM-POSTE-<chipId>`, ouvrir
   `http://192.168.4.1` et choisir le même réseau local.
7. Dans **Gestion des postes**, attendre la détection du `chipId`, puis attribuer
   un nom au poste.
8. Simuler un coin ou insérer une pièce, affecter le crédit au poste et vérifier
   que le relais s'active et que le temps diminue.

## Persistance et redémarrage

| Appareil | Namespace NVS | Données conservées |
|---|---|---|
| Centrale | `wifi` | SSID, mot de passe et configuration DHCP ou IPv4 statique. |
| Centrale | `appcfg` | Durée par coin, impulsions par coin et solde. |
| Centrale | `auth` | Comptes, hashes salés et token API. |
| Centrale | `posts` | Identité et dernière IP des postes configurés. |
| Poste | `wifi` | SSID, mot de passe et configuration DHCP ou IPv4 statique. |
| Poste | `identity` | Identifiant logique et nom du poste. |
| Poste | `session` | Dernier temps restant sauvegardé ; une valeur positive indique une session interrompue. |

Ne sont pas persistants : les logs et sessions web de la centrale, les postes
en attente et les statuts instantanés. Après le redémarrage d'un poste interrompu,
son relais reste coupé, mais le dernier temps sauvegardé est proposé à la
centrale pour une relance ou une annulation explicite.

## Rôle de chaque fichier

### Firmware de la centrale

| Fichier | Rôle |
|---|---|
| `central-esp32/central-esp32.ino` | Point d'entrée Arduino : crée l'état global et le serveur, initialise les services et exécute leur boucle. |
| `central-esp32/AppConfig.h` | Regroupe les valeurs par défaut, secrets, GPIO, port et délais. |
| `central-esp32/AppState.h` | Définit l'état partagé en RAM : crédits, impulsions, comptes, sessions, logs et postes. |
| `central-esp32/Post.h` | Définit la structure d'un poste configuré, son dernier état connu et une éventuelle reprise en attente. |
| `central-esp32/AuthService.h` | Déclare l'API interne d'authentification et de gestion des utilisateurs. |
| `central-esp32/AuthService.cpp` | Gère les comptes, rôles, hashes de mots de passe, cookies de session et tokens Bearer. |
| `central-esp32/CoinService.h` | Déclare le service de lecture du monnayeur. |
| `central-esp32/CoinService.cpp` | Traite l'interruption GPIO, l'anti-rebond et la conversion des impulsions en coins persistants. |
| `central-esp32/LogService.h` | Déclare le service de journalisation. |
| `central-esp32/LogService.cpp` | Écrit sur le port série et conserve les 100 derniers événements en RAM. |
| `central-esp32/PostService.h` | Déclare la logique métier de gestion des postes. |
| `central-esp32/PostService.cpp` | Gère découverte, configuration, crédits, arrêt, reprise après coupure, suppression et état hors ligne. |
| `central-esp32/PosteClient.h` | Déclare le client HTTP utilisé pour joindre un poste. |
| `central-esp32/PosteClient.cpp` | Envoie les commandes et configurations au poste, et décode la réponse de `/status`. |
| `central-esp32/StorageService.h` | Déclare la persistance et la sauvegarde/restauration complète. |
| `central-esp32/StorageService.cpp` | Lit et écrit les namespaces NVS et sérialise les données persistantes en JSON. |
| `central-esp32/WifiService.h` | Déclare la connexion normale au réseau et le démarrage mDNS. |
| `central-esp32/WifiService.cpp` | Utilise le Wi-Fi enregistré et publie le service HTTP `pstm.local`. |
| `central-esp32/WifiProvisioning.h` | Déclare le mode de configuration Wi-Fi. |
| `central-esp32/WifiProvisioning.cpp` | Implémente le point d'accès, le DNS captif, le scan, le formulaire et la sauvegarde Wi-Fi. |
| `central-esp32/WebRoutes.h` | Déclare l'enregistrement des routes du serveur web. |
| `central-esp32/WebRoutes.cpp` | Implémente les pages, l'API JSON, les autorisations et la réception des annonces. |
| `central-esp32/WebPage.h` | Déclare les générateurs des pages web embarquées. |
| `central-esp32/WebPage.cpp` | Contient le HTML, le CSS et le JavaScript de la connexion et de l'interface d'administration. |
| `central-esp32/WebAssets.h` / `central-esp32/WebAssets.cpp` | Expose les images optimisées intégrées au firmware. |
| `central-esp32/*Asset.inc` | Contient les données binaires `PROGMEM` générées depuis les assets optimisés. |
| `central-esp32/ui-assets/` | Regroupe les sources graphiques maîtres et les variantes allégées documentées. |
| `central-esp32/README.md` | Manuel détaillé du firmware central, de son interface et de son API. |

### Firmware d'un poste

| Fichier | Rôle |
|---|---|
| `poste-esp32/poste-esp32.ino` | Point d'entrée Arduino : initialise identité, relais, serveur, Wi-Fi et annonces. |
| `poste-esp32/PosteConfig.h` | Regroupe Wi-Fi de secours, portail, mDNS, secret partagé, GPIO relais, polarité et intervalle de sauvegarde. |
| `poste-esp32/PosteState.h` | Définit l'état partagé en RAM : identité, connexion, session, reprise en attente et relais. |
| `poste-esp32/PosteIdentityService.h` | Déclare la gestion de l'identité persistante. |
| `poste-esp32/PosteIdentityService.cpp` | Calcule le `chipId` eFuse et charge, sauvegarde ou efface `id` et `name` dans la NVS. |
| `poste-esp32/RelayService.h` | Déclare le pilotage du relais et des sessions. |
| `poste-esp32/RelayService.cpp` | Pilote le relais, gère les sessions et sauvegarde/restaure le temps récupérable après une coupure. |
| `poste-esp32/PosteAnnounceService.h` | Déclare l'annonce périodique vers la centrale. |
| `poste-esp32/PosteAnnounceService.cpp` | Résout la centrale par mDNS et lui envoie l'identité et l'état du poste en JSON. |
| `poste-esp32/PosteWebServer.h` | Déclare les routes HTTP propres au poste. |
| `poste-esp32/PosteWebServer.cpp` | Implémente `/status`, `/configure`, `/identity/clear`, `/command` et la validation Bearer. |
| `poste-esp32/WiFiConfigService.h` | Déclare l'initialisation et le suivi du Wi-Fi. |
| `poste-esp32/WiFiConfigService.cpp` | Gère les identifiants Wi-Fi, la connexion, le point d'accès, le portail captif et son DNS. |
| `poste-esp32/README.md` | Manuel détaillé du firmware de poste, de son câblage et de son API. |

Les fichiers `.h` définissent les structures et interfaces publiques. Les
fichiers `.cpp` portent leur implémentation, tandis que chaque `.ino` orchestre
le démarrage et la boucle principale de son ESP32.

## Sécurité avant déploiement

Modifier impérativement les valeurs de développement :

- le mot de passe administrateur `12345678` ;
- si le portail de configuration doit être protégé, `AP_PASSWORD`, vide par
  défaut sur la centrale et le poste ;
- le secret partagé `POSTE_COMMAND_TOKEN` / `COMMAND_TOKEN` ;
- le token API de la centrale, depuis l'interface ;
- éventuellement le mot de passe du portail Wi-Fi des postes, ouvert par défaut.

Le système utilise HTTP sans TLS. Il doit rester sur un réseau local privé, sans
redirection de port depuis Internet. Le token partagé et les identifiants Wi-Fi
sont présents dans le firmware ou la NVS sans chiffrement applicatif.

## Limites importantes

- une reprise après coupure nécessite une décision manuelle et n'est précise
  qu'à l'intervalle du dernier instantané, 60 secondes par défaut ;
- le temps écoulé pendant la coupure n'est pas déduit sans horloge RTC secourue ;
- raccourcir fortement l'intervalle de sauvegarde augmente l'usure de la flash ;
- la résolution de `pstm.local` dépend du support mDNS du réseau ;
- les postes sont interrogés séquentiellement, donc un tour complet prend
  environ cinq secondes multipliées par leur nombre ;
- les logs de la centrale sont limités à 100 entrées et perdus au redémarrage ;
- le serveur Arduino convient à un petit réseau local, pas à une exposition
  publique ou à une forte charge ;
- le changement du secret inter-ESP32 exige de recompiler la centrale et tous
  les postes avec exactement la même valeur.
