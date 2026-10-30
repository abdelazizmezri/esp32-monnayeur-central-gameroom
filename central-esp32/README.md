# Centrale ESP32 - PS TIME MANAGER

Firmware de la centrale d'un système de gestion de postes de jeu à monnayeur.
La centrale ESP32 reçoit les impulsions du monnayeur, transforme ces impulsions
en crédits, découvre les ESP32 installés sur les postes et distribue du temps de
jeu depuis une interface web locale.

## Guides PDF

- [Guide technique client](../output/pdf/ps-time-manager-guide-technique-client.pdf)
  pour l'installation, le câblage, le téléversement et la recette technique ;
- [Manuel d'utilisation](../output/pdf/ps-time-manager-manuel-utilisation.pdf)
  pour les parcours administrateur et utilisateur simple.

Les captures de ces guides utilisent des données fictives ; elles ne contiennent aucun SSID,
compte, mot de passe, token ou historique provenant d'une centrale réelle.

## Prise en charge TND / DZD

La [notice multidevise](../docs/multidevise.md) décrit les pièces,
les sessions de caisse à devise unique, le format REV2, la migration, les
sauvegardes V4 et la maintenance de l’historique. La devise ne modifie pas les crédits ou la durée de jeu.
Les anciens exemples en millimes de ce manuel décrivent le mode TND.

## Documentation actuelle et captures

Le [manuel illustré](../docs/manuals/manuel-utilisation.md) décrit les tâches
de l'utilisateur simple et de l'administrateur, avec des procédures numérotées.
Le [guide technique](../docs/manuals/guide-technique.md) accompagne l'installation
des outils, le câblage, la programmation et la mise en réseau.

![Tableau de bord actuel de la centrale](../output/screenshots/interface-accueil.png)

*Rendu navigateur réel des pages embarquées, données de démonstration : tarifs
1/4/1/2, cinq widgets et quatre états de poste.*

![Configuration monétaire et sauvegarde](../output/screenshots/interface-configuration.png)

*La durée est commune ; le tarif se règle dans Gestion des postes. Le panneau
voisin exporte ou sélectionne un fichier JSON V4.*

## Sommaire

- [Guides PDF](#guides-pdf)
- [Fonctions principales](#fonctions-principales)
- [Architecture](#architecture)
- [Démarrage du firmware](#démarrage-du-firmware)
- [Gestion des coins](#gestion-des-coins)
- [Recette dans LittleFS](#recette-dans-littlefs)
- [Câblage final de la centrale](#câblage-final-de-la-centrale)
- [Gestion des postes](#gestion-des-postes)
- [Gestion des utilisateurs](#gestion-des-utilisateurs)
- [Interface web](#interface-web)
- [Installation et compilation](#installation-et-compilation)
- [Manuel d'utilisation](#manuel-dutilisation)
- [API HTTP](#api-http)
- [Persistance des données](#persistance-des-données)
- [Rôle de chaque fichier](#rôle-de-chaque-fichier)
- [Sécurité](#sécurité)
- [Dépannage](#dépannage)
- [Limites connues](#limites-connues)

## Fonctions principales

- configuration Wi-Fi par point d'accès et portail captif ;
- accès local par adresse IP ou `http://pstm.local` ;
- lecture du monnayeur sur le GPIO 13 ;
- une pièce disponible par impulsion et un tarif configurable pour chaque poste ;
- devise de caisse TND ou DZD, valeur d'impulsion configurable et clôture de session ;
- crédit disponible sauvegardé en mémoire NVS ;
- journal monétaire binaire compact dans LittleFS ;
- recette par jour, semaine ou mois avec graphique, cartes, tableau et exports PDF/CSV ;
- découverte automatique des ESP32 des postes ;
- configuration, renommage et suppression des postes ;
- démarrage, prolongation et arrêt des sessions ;
- détection et traitement indépendant des sessions interrompues après coupure ;
- surveillance de l'état, du relais et du temps restant ;
- interface web responsive embarquée dans le firmware ;
- comptes multiples avec rôles `admin` et `user` ;
- journal des 100 derniers événements ;
- sauvegarde et restauration complètes des réglages et données persistantes ;
- API HTTP protégée par session ou token Bearer.

## Architecture

```mermaid
flowchart LR
    Coin[Monnayeur] -->|Impulsions GPIO 13| Central[Centrale ESP32]
    Admin[Navigateur administrateur] -->|HTTP local| Central
    User[Navigateur utilisateur simple] -->|HTTP local| Central
    Central -->|Commandes HTTP| Post1[ESP32 Poste 1]
    Central -->|Commandes HTTP| Post2[ESP32 Poste 2]
    Post1 -->|Annonce et statut| Central
    Post2 -->|Annonce et statut| Central
    Post1 --> Display1[Afficheur TM1637]
    Post2 --> Display2[Afficheur TM1637]
    Central --> NVS[(Mémoire NVS)]
    Central --> LFS[(LittleFS interne\nrecettes)]
```

La centrale est le point de décision du système. Les postes doivent exécuter le
firmware compatible du dossier voisin `../poste-esp32`, qui fournit les
endpoints `/status`, `/command`, `/configure` et `/identity/clear`, commande le
relais et affiche localement le temps restant sur un TM1637. Son contrôle de
session fonctionne dans une tâche FreeRTOS dédiée : les délais mDNS ou HTTP ne
peuvent plus figer l'affichage ni retarder l'expiration du relais.

### Composants internes

| Composant | Responsabilité |
|---|---|
| Point d'entrée Arduino | Initialise les services et exécute la boucle principale. |
| Wi-Fi | Connexion au réseau existant ou ouverture du portail de configuration. |
| Monnayeur | Compte les impulsions et alimente le crédit central. |
| Gestion des postes | Découverte, identité, sessions et surveillance. |
| Client des postes | Échanges HTTP entre la centrale et chaque poste. |
| Authentification | Comptes, mots de passe, sessions et permissions. |
| Stockage | Configuration et solde en NVS ; historique des recettes dans LittleFS. |
| Recette | Horodatage NTP, journal des impulsions, agrégation et exports. |
| Web | Pages HTML/CSS/JavaScript et routes de l'API. |
| Logs | Journal en RAM et sortie série. |

## Démarrage du firmware

La fonction `setup()` exécute les opérations suivantes :

1. ouverture du port série à 115200 bauds ;
2. chargement de la configuration NVS ;
3. création ou migration du compte administrateur ;
4. tentative de connexion au Wi-Fi enregistré pendant 15 secondes ;
5. démarrage du portail Wi-Fi si la connexion échoue ;
6. sinon, démarrage de mDNS et de la synchronisation NTP ;
7. montage de LittleFS, puis démarrage du monnayeur, des postes et du serveur web.

En mode normal, `loop()` traite continuellement :

- les requêtes HTTP ;
- les impulsions reçues du monnayeur ;
- l'état de l'horloge et les nouvelles tentatives de montage LittleFS ;
- l'expiration des postes en attente ;
- l'interrogation périodique des postes configurés.

### Mode de configuration Wi-Fi

Si aucun Wi-Fi valide n'est enregistré, la centrale crée ce point d'accès :

| Paramètre | Valeur par défaut |
|---|---|
| SSID | `GAMEROOM-CENTRAL-<chipId>` |
| Mot de passe | Aucun (réseau ouvert par défaut) |
| Adresse du portail | `http://192.168.4.1` |

Le portail scanne les réseaux disponibles et propose deux modes d'adressage :

- **Automatique (DHCP)**, utilisé par défaut et compatible avec les anciennes
  configurations enregistrées ;
- **Manuel (IP fixe)**, avec adresse IPv4, passerelle, masque de sous-réseau et
  DNS principal/secondaire facultatifs.

Les identifiants et les paramètres réseau sont validés puis enregistrés
uniquement après une connexion réussie. La centrale redémarre ensuite avec la
même configuration. Il n'existe pas de proxy HTTP système sur l'ESP32 : les
communications avec les postes et mDNS restent directes sur le réseau local.
Le suffixe `chipId`, calculé depuis l'eFuse, distingue aussi plusieurs centrales
qui seraient simultanément en mode configuration.

## Gestion des coins

Le signal `COIN`, adapté en logique 0–3,3 V, arrive sur le GPIO 13 configuré en
`INPUT_PULLUP`. Le rappel interne vers **3V3** rétablit le niveau HIGH quand une
sortie à collecteur ouvert vers GND est relâchée. Ce montage ne nécessite pas de
résistance externe de rappel : retirer toute résistance de 2 kΩ ou 4,7 kΩ entre
GPIO13 et GND, alimentations coupées. Pour une liaison non isolée, la masse du
monnayeur et celle de l'ESP32 sont communes. Confirmer que la sortie ne fournit
aucune tension supérieure à 3,3 V, ou utiliser une interface adaptée au monnayeur.
Le nom JY-133B et le réglage NO/NC ne suffisent pas à garantir le type électrique
de la sortie. Une résistance pull-down seule ne convertit pas du 5 V ou du 12 V
en 3,3 V. Ne jamais relier le `+12 V` au GPIO13.

Comme dans le projet de référence fonctionnel, l'entrée est active à LOW.
Le réglage **NO** est conseillé : repos HIGH, impulsion LOW. Le firmware compte
un front descendant par interruption, sans attendre la fin de l'impulsion et
sans limite de durée minimale ou maximale. Après un front accepté, un verrouillage
anti-doublon de **320 ms** ignore les fronts suivants avant qu'ils n'alimentent
les crédits ou la recette, sans attente bloquante. Les fronts ignorés ne prolongent
pas ce verrouillage. Il reprend le délai effectif du projet de référence :
`timer = 20` tours de boucle × 4 chiffres × `delay(4)`, soit environ 320 ms,
et non 20 ms. Ce réglage vise une impulsion utile par pièce.
Un niveau maintenu LOW ne crée pas de nouveaux crédits tant qu'il n'y a pas
de nouveau passage de HIGH à LOW.

Au démarrage du service, le comptage est désarmé pendant **5 secondes**, comme
pendant l'animation du projet de référence. Les transitions électriques de
mise en route ne créent ni crédit ni recette. Ensuite, l'entrée doit être
revenue au repos HIGH avant l'armement. Cette protection est non bloquante :
le serveur et les communications des postes continuent de fonctionner.

Le journal web et le moniteur série à 115200 bauds affichent d'abord
`Monnayeur en stabilisation`, avec le niveau GPIO et le solde initial, puis
`Monnayeur prêt` après armement, avec la broche, `rappel=pull-up interne`,
la détection sur front descendant, `repos=HIGH` et l'anti-doublon.
Attendre ce second message avant d'introduire une pièce. Une interruption
retardée est ignorée si la broche est déjà revenue à HIGH.
Chaque lot d'impulsions comptées
produit ensuite `Impulsions monnayeur validées`, avec les pièces créditées et le solde.
La recette, le stockage et les logs sont traités dans la boucle principale,
jamais dans l'interruption. Chaque impulsion finalisée est persistée sans
attendre d'autres impulsions. Le mode de configuration Wi-Fi ne démarre pas le comptage.

Il faut distinguer quatre notions :

- une **impulsion** est le signal électrique validé et correspond à une valeur monétaire ;
- une **pièce disponible** est une unité de crédit ajoutée par une impulsion ;
- le **tarif du poste** est son nombre de pièces requis pour une attribution ;
- la **recette** compte uniquement l'argent des impulsions physiques validées.

```text
recette ajoutée = impulsions validées × valeur d'une impulsion
pièces disponibles ajoutées = impulsions validées
pièces débitées par clic = tarif du poste
durée attribuée par clic = durée par attribution
```

Un poste à 1 pièce affiche **+1** et un poste à 4 affiche **+4**.
Chaque clic ajoute une seule durée de jeu, même lorsqu'il consomme 4 pièces.
Le tarif se configure lors de l'ajout du poste puis dans sa liste de gestion.
Il reste modifiable en cours de session sans toucher au temps déjà attribué.
La durée commune reste dans **Configuration**, avec la devise, la valeur par
impulsion et la session de caisse. Il n'existe plus de conversion globale des coins.

Les nouveaux postes coûtent 1 pièce ; la durée initiale est de 1 800 secondes et
le solde de 0 pièce. Les tarifs et le solde sont persistants. Les anciens coins
et reliquats sont migrés une seule fois, sans modifier la recette historique.
Voir les [règles de migration, sauvegardes V4 et reprise après coupure](../docs/tarification-par-poste.md).

Un monnayeur multi-pièces peut produire plusieurs impulsions par pièce physique.
La valeur monétaire choisie représente une impulsion. Pour ces trains multiples,
adapter `COIN_DEBOUNCE_MS` à l'espacement mesuré ; les 320 ms actuelles visent une
impulsion utile par pièce physique.

> Attention : les GPIO de l'ESP32 fonctionnent en 3,3 V. Ne jamais injecter
> directement une tension supérieure. Mesurer la sortie réelle et confirmer le
> brochage du modèle de monnayeur avant la mise sous tension.

## Recette dans LittleFS

La centrale synchronise son horloge par NTP avec le fuseau tunisien `CET-1`,
puis ajoute chaque lot d'impulsions dans `/revenue.bin`, dans la partition
LittleFS de la flash interne. Le format REV2 utilise des enregistrements de
72 octets : date, devise, valeur d'impulsion, montant natif, session de caisse,
compteurs, séquence, instantané des crédits et CRC16. Les anciens champs de taux
et de conversion restent réservés pour lire les archives, sans participer aux calculs. L'ancien format REV1
(16 octets) est migré sans modifier les montants TND. Une dernière écriture
incomplète est réparée ; une corruption intérieure bloque le journal et conserve
l'original pour intervention.

La valeur monétaire est écrite au moment de l'encaissement. Une modification
ultérieure du tarif ne change donc jamais l'historique. Les corrections
administratives du solde ne sont pas comptabilisées comme recette.

La page administrateur `/recette` propose :

- une sélection de session active ou archivée, puis **Jour**, **Semaine** ou **Mois**, avec date de référence ;
- cinq cartes : recette de la période, total de la session, impulsions, crédits historiques / pièces et écritures ;
- un graphique en barres et un tableau détaillé ;
- un export CSV agrégé côté ESP32 ;
- un export PDF généré localement dans le navigateur.
- le choix de session active ou archivée dans sa devise d’origine ;
- **Mémoire de recette > Exporter et vider la recette** pour exporter toutes les
  sessions, vérifier le PDF puis confirmer le remplacement du seul journal.

Le PDF présente une synthèse et des tableaux paginés ; le CSV UTF-8 utilise le
point-virgule et distingue la synthèse du détail. Le bouton Actualiser est séparé
des exports. Une nouvelle opération après l’export ou une autorisation expirée
invalide l’effacement. Les crédits, paramètres et autres fichiers restent conservés.

Si une pièce est introduite avant la synchronisation NTP, l'écriture reçoit un
epoch nul. Elle apparaît comme non datée et n'est pas affectée arbitrairement à
un jour. Si LittleFS devient indisponible, une transaction préparée reste en NVS
et les crédits correspondants attendent sa finalisation. Les impulsions suivantes
restent dans une file RAM bornée ; l'interface et les logs signalent l'échec de
stockage. La centrale retente le montage toutes les 30 secondes.

Au tout premier démarrage avec la nouvelle partition, LittleFS est formaté puis
marqué comme initialisé dans la NVS. Aux démarrages suivants, un échec de montage
ne déclenche jamais un reformatage automatique : l'historique est protégé et
l'administrateur reçoit une erreur explicite.

La sauvegarde JSON contient la configuration monétaire, mais pas le journal de
recette. LittleFS et la NVS ont des rôles séparés : NVS conserve les paramètres,
le solde, les comptes et les postes ; LittleFS conserve les événements financiers.

## Câblage final de la centrale

> Ce schéma utilise le pull-up interne de GPIO13 pour une sortie à collecteur
> ouvert vers GND ou une interface compatible 0–3,3 V vérifiée. Une sortie à
> tension plus élevée nécessite une adaptation avant raccordement.

![Schéma de câblage final de la centrale ESP32 sans module mémoire](docs/schema-cablage-central-final.svg)

Le monnayeur reste alimenté en 12 V et l'ESP32 est alimenté séparément par USB.
Le schéma utilise le rappel interne vers 3V3, sans résistance externe vers GND.
NVS et LittleFS sont des zones de la flash intégrée à
l'ESP32 : aucun module mémoire externe n'est nécessaire.

Le schéma utilise des fils colorés et les noms sérigraphiés des broches, dans le
style d'un montage physique. La position exacte des broches peut varier selon le
fabricant du DevKit : toujours suivre les étiquettes `VIN/5V`, `GND`, `3V3` et
`GND` et `GPIO13` imprimées sur la carte, et non uniquement leur position dans
le dessin.

| Depuis | Vers | Rôle |
|---|---|---|
| Alimentation `+12 V` | Fil rouge du monnayeur | Alimentation du monnayeur |
| Alimentation `GND` | Fil noir du monnayeur et `GND` de l'ESP32 | Masse commune |
| Fil blanc `COIN` du monnayeur | ESP32 `GPIO13` | Signal impulsionnel, mesuré à 3,3 V maximum |
| Pull-up interne `INPUT_PULLUP` | GPIO13 vers 3V3, dans l'ESP32 | Activé par le firmware ; aucune résistance externe vers GND |

Les broches SPI (`MOSI`, `MISO`, `SCK`, `CS`) restent libres : le journal de
recette utilise LittleFS dans la flash interne et ne demande aucun câblage.

## Gestion des postes

### Découverte

Chaque ESP32 de poste annonce périodiquement son identité à la centrale avec
`POST /poste/announce` et le token partagé des postes.

- Un poste sans identité apparaît dans **Postes découverts**.
- Un poste configuré est ajouté ou actualisé dans la liste permanente.
- Une découverte non configurée disparaît après 30 secondes sans annonce.

### Configuration d'un nouveau poste

L’administrateur renseigne **Nom du poste** et **Nombre de pièces requis** dans
la carte découverte, puis clique sur **Ajouter ce poste**. Le tarif vaut 1 par
défaut et doit être un entier positif. La centrale reprend automatiquement le
`chipId` matériel comme identifiant technique ; la carte découverte l’affiche
à titre d’information, sans champ de saisie de cet identifiant.

La centrale envoie l'identité technique et le nom à `POST http://IP/configure`.
Le poste est ensuite sauvegardé dans la NVS de la centrale.

### Sessions de jeu

Lors de l’attribution de temps :

- la centrale envoie `START_SESSION` si le poste est inactif ;
- elle envoie `ADD_TIME` si le poste est déjà actif ;
- le bouton **Arrêter** envoie `STOP_SESSION`.

Si un poste redémarre pendant une session, son firmware compatible annonce
`recoveryPending=true` avec `recoveryRemaining`. La centrale affiche alors une
proposition de reprise propre à ce poste. L'opérateur peut reprendre le temps
sauvegardé ou l'annuler définitivement.

Le relais du poste reste coupé tant qu'aucune décision n'a été envoyée. Les
reprises de plusieurs postes sont affichées et traitées indépendamment. Sur le
poste, le temps récupérable clignote sur le TM1637 jusqu'à cette décision.

Le poste confirme une commande après l'avoir appliquée à son état protégé. Les
crédits restent donc débités uniquement après une réponse réussie, comme avant
la séparation temps réel/réseau.

### Surveillance

La centrale interroge un poste toutes les 5 secondes avec `GET /status`. Les
postes sont interrogés à tour de rôle. Après un échec et plus de 10 secondes sans
réponse valide, le poste passe à l'état `offline`.

En complément de ce sondage, chaque poste conserve un battement d'annonce de
5 secondes et envoie plus rapidement les transitions de session. Une révision
interne empêche qu'un état devenu ancien pendant la transmission reste affiché :
le poste renvoie le nouvel instantané dès la fin de l'échange précédent.

Les informations affichées sont :

- identifiant et nom ;
- adresse IP ;
- état `idle`, `active`, `recovery_pending`, `offline`, `error` ou `unknown` ;
- état du relais ;
- temps restant.

Un poste actif, possédant encore du temps ou attendant une décision de reprise
ne peut pas être modifié ou supprimé.

## Gestion des utilisateurs

### Compte par défaut

| Champ | Valeur |
|---|---|
| Nom d'utilisateur | `admin` |
| Mot de passe | `12345678` |
| Rôle | Administrateur |

Au premier démarrage après mise à jour d'une ancienne version, l'ancien mot de
passe administrateur est migré vers le compte `admin`.

### Données d'un compte

Un utilisateur possède :

- un nom d'utilisateur unique, normalisé en minuscules ;
- un prénom ;
- un nom ;
- un mot de passe d'au moins 6 caractères pour les nouveaux comptes ;
- un rôle `admin` ou `user`.

Les mots de passe sont sauvegardés sous forme de hash SHA-256 avec un sel aléatoire.
Ils ne sont jamais renvoyés par les routes ordinaires. La sauvegarde complète
contient toutefois les hashes et les sels afin que les comptes puissent être
restaurés avec leurs mots de passe actuels ; le fichier exporté doit donc être
conservé dans un emplacement sûr.

### Permissions

| Fonction | Administrateur | Utilisateur simple |
|---|:---:|:---:|
| Consulter les postes et leur état | Oui | Oui |
| Attribuer du temps au tarif du poste | Oui | Oui |
| Arrêter un poste | Oui | Oui |
| Relancer ou annuler une session interrompue | Oui | Oui |
| Configurer, modifier ou supprimer un poste | Oui | Non |
| Modifier la configuration générale | Oui | Non |
| Consulter ou vider les logs | Oui | Non |
| Gérer les utilisateurs | Oui | Non |
| Modifier le Wi-Fi | Oui | Non |
| Gérer le token API | Oui | Non |

La centrale accepte jusqu'à 10 comptes. Elle empêche la suppression de son
propre compte, la modification de son propre rôle et la suppression ou la
rétrogradation du dernier administrateur.

## Interface web

La page `/login` reprend la mise en page en deux panneaux de la maquette : la
photographie HD de la salle à gauche, le logo PS TIME MANAGER HD et le formulaire
sécurisé à droite. Le fond 1123 × 1401 px, le logo 720 × 240 px et le
pictogramme PlayStation 128 × 97 px sont embarqués en WebP. Les icônes de
navigation, d'état et d'action restent des SVG intégrés, donc indépendants de la
résolution de l'écran.

### Pages accessibles à tous les comptes connectés

- `/` : tableau de bord, état des postes, pièces disponibles et commandes autorisées ;
- `/login` : connexion ;
- déconnexion depuis la barre latérale.

Le tableau de bord propose **Masquer les cartes** / **Afficher les cartes** pour
replier ensemble les cinq indicateurs situés au-dessus de la liste des postes.
La préférence est conservée dans le navigateur, y compris après une
actualisation ou une reconnexion, tandis que les valeurs continuent d'être
actualisées en arrière-plan.

### Pages réservées aux administrateurs

- `/config` : durée par attribution, solde en pièces et sauvegarde/restauration V4 ;
- `/recette` : recette du jour, de la semaine ou du mois et exports PDF/CSV ;
- `/logs` : journal des événements ;
- `/postes` : postes configurés et postes découverts ;
- `/users` : création et administration des comptes ;
- `/security` : mot de passe de l'administrateur et token API.

Les permissions sont contrôlées côté serveur. Masquer un bouton dans le
navigateur ne suffit donc pas à contourner les restrictions.

## Installation et compilation

### Matériel et logiciels

| Catégorie | Éléments requis |
|---|---|
| Carte | ESP32 Dev Module avec 4 Mo de flash, câble USB de données. |
| Monnayeur | JY-133B ou équivalent, alimentation 12 V et sortie compatible 3,3 V ou interface adaptée. Pull-up interne activé ; ESP32 alimenté séparément par USB. |
| Outils | Ordinateur, multimètre, tournevis, pince à dénuder/sertir et matériel de soudure isolé. |
| Logiciel | Arduino IDE 2.3.10 ou `arduino-cli` 1.5.1. |
| Cœur de cartes | **esp32 by Espressif Systems** 3.3.12, cible `esp32:esp32:esp32`. |
| Bibliothèque externe | **ArduinoJson** 7.4.3. |

`WiFi`, `WebServer`, `DNSServer`, `Preferences`, `ESPmDNS`, `HTTPClient`,
`FS`, `LittleFS` et mbedTLS sont fournis par le cœur ESP32 3.3.12 et ne doivent
pas être installés séparément. Après compilation, vérifier que la taille du
programme ne dépasse pas les **1 703 936 octets** de la partition applicative
définie dans `partitions.csv`.

### Avec Arduino IDE

1. Ouvrir `central-esp32.ino` directement depuis ce dossier. Conserver
   `partitions.csv` dans le même dossier que le fichier `.ino`.
2. Installer ArduinoJson depuis le gestionnaire de bibliothèques.
3. Ouvrir **Tools/Outils → Board/Type de carte → esp32** et sélectionner une
   carte compatible, par exemple **ESP32 Dev Module**.
4. Régler **Tools/Outils → Flash Size → 4MB (32Mb)**.
5. Ouvrir **Tools/Outils → Partition Scheme/Schéma de partition** et choisir
   **Minimal SPIFFS**, profil `min_spiffs`. Le suffixe du libellé peut varier
   selon la version du paquet de cartes.
6. Régler **Erase All Flash Before Sketch Upload → Disabled**, sauf si
   l'effacement volontaire de toute la NVS est souhaité.
7. Sélectionner le port série de la centrale.
8. Cliquer sur **Vérifier**, puis **Téléverser** afin d'écrire le firmware et la
   nouvelle table de partitions par USB.
9. Ouvrir le moniteur série à 115200 bauds.

Conserver également **CPU Frequency = 240 MHz (WiFi/BT)**, **Flash Frequency =
80 MHz**, **Flash Mode = QIO** et **PSRAM = Disabled**. Une vitesse d'upload de
460800 convient généralement ; utiliser 115200 si la liaison série est instable.

Le profil du menu fournit au compilateur une limite de taille suffisante pour
l'application. Le fichier local `partitions.csv`
remplace automatiquement le découpage standard par deux applications OTA de
1,625 Mio et **640 Kio de LittleFS**. Ne pas choisir **Default**, et ne pas
renommer, déplacer ou supprimer `partitions.csv`.

Le fichier `partitions.csv` garantit deux partitions OTA de 1,625 Mio, une
partition LittleFS de 640 Kio et la NVS à son emplacement actuel. Après cette
mise à jour, faire un téléversement complet par USB pour écrire la table de
partitions. Éviter **Erase All Flash** si les paramètres NVS doivent être
conservés. La nouvelle table reformate LittleFS à sa première initialisation.

| Partition | Offset | Taille |
|---|---:|---:|
| `nvs` | `0x9000` | 20 Kio |
| `otadata` | `0xE000` | 8 Kio |
| `app0` | `0x10000` | 1,625 Mio |
| `app1` | `0x1B0000` | 1,625 Mio |
| `littlefs` | `0x350000` | 640 Kio |
| `coredump` | `0x3F0000` | 64 Kio |

La taille maximale physique du firmware dans `app0` est **1 703 936 octets**.
Le profil de menu Minimal SPIFFS annonce une limite plus grande ; après toute
modification, comparer la
taille réelle indiquée en fin de compilation à 1 703 936 octets. La présence de
deux emplacements applicatifs ne signifie pas qu'une mise à jour OTA est
disponible : ce firmware se téléverse par USB.

### Avec arduino-cli

```bash
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli lib install ArduinoJson@7.4.3
arduino-cli board list
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs .
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs --port PORT_SERIE .
```

Adapter `PORT_SERIE`, par exemple `/dev/cu.usbserial-0001` sous macOS ou
`COM3` sous Windows.

## Manuel d'utilisation

### 1. Première mise en service

1. Alimenter la centrale et ouvrir le moniteur série.
2. Se connecter au Wi-Fi `GAMEROOM-CENTRAL-<chipId>` affiché dans le moniteur série.
3. Le point d'accès est ouvert par défaut. Si `AP_PASSWORD` a été défini avant
   compilation, utiliser ce mot de passe.
4. Ouvrir `http://192.168.4.1` si le portail ne s'affiche pas automatiquement.
5. Sélectionner le réseau local partagé par la centrale et les postes.
6. Saisir son mot de passe et valider.
7. Attendre le redémarrage automatique.
8. Reconnecter le téléphone ou le PC au réseau local.
9. Ouvrir l'adresse IP affichée ou `http://pstm.local`.

### 2. Première connexion

1. Saisir le nom d'utilisateur `admin`.
2. Saisir le mot de passe `12345678`.
3. Aller immédiatement dans **Mot de passe / Token**.
4. Choisir un nouveau mot de passe d'au moins 6 caractères.
5. Se reconnecter avec le nouveau mot de passe.

### 3. Ajouter un utilisateur simple

1. Ouvrir **Utilisateurs**.
2. Saisir un nom d'utilisateur unique, le prénom et le nom.
3. Définir un mot de passe d'au moins 6 caractères.
4. Sélectionner **Utilisateur simple**.
5. Cliquer sur **Ajouter**.

### 4. Ajouter un autre administrateur

Suivre la même procédure en sélectionnant **Administrateur**. Il est recommandé
de disposer d'au moins deux comptes administrateurs avant toute opération de
maintenance importante.

### 5. Ajouter un poste

1. Mettre le poste ESP32 compatible sous tension sur le même réseau.
2. Ouvrir **Gestion des postes**.
3. Attendre son apparition dans **Postes découverts**.
4. Saisir **Nom du poste** et **Nombre de pièces requis** (1 par défaut).
5. Cliquer sur **Ajouter ce poste**. L’identifiant technique est dérivé
   automatiquement du `chipId` matériel.
6. Vérifier que le poste apparaît dans la liste configurée.

### 6. Attribuer du temps

1. Vérifier le crédit disponible en haut du tableau de bord.
2. Repérer le poste voulu.
3. Cliquer sur le bouton du poste, par exemple **+1** ou **+4** : une seule durée est ajoutée.
4. Vérifier le passage à l'état `active`, le relais `ON` et le décompte local sur
   l'afficheur TM1637 du poste.

Si le poste est déjà actif, le temps est ajouté à la session existante.

### 7. Arrêter un poste

Cliquer sur **Arrêter** sur la carte du poste. Le poste reçoit `STOP_SESSION` et
son état local est immédiatement remis à `idle`, relais `OFF`, temps restant 0.

### 8. Traiter une session interrompue

Après le redémarrage d'un poste qui était actif :

1. repérer la carte marquée **Reprise en attente** ;
2. vérifier le temps sauvegardé avant la coupure ;
3. cliquer sur **Reprendre** et confirmer pour reprendre le temps sauvegardé ;
4. ou cliquer sur **Annuler** et confirmer pour effacer le temps sans relancer.

L’interface actuelle ne propose pas de champ de minutes offertes. L’API conserve
son paramètre facultatif `extraMinutes`. Une attribution supplémentaire se fait
ensuite avec le bouton **+N**, au tarif du poste.

### 9. Modifier la configuration monétaire

Dans **Configuration** :

- **Durée par attribution** : nombre de minutes ajoutées par clic ;
- le **Nombre de pièces requis** se règle dans **Gestion des postes**, à la création et dans **Modifier** ;
- **Devise et valeur d'une impulsion** : valeurs TND existantes ou pièces DZD ;
- **Clôture de caisse** : ouvrir une nouvelle session à zéro et conserver la précédente dans l'historique ;
- **Crédit disponible** : correction administrative du solde.

Cliquer sur **Enregistrer** après modification.

### 10. Consulter et exporter la recette

1. Ouvrir **Recette** avec un compte administrateur.
2. Choisir **Jour**, **Semaine** ou **Mois**, puis une date de référence.
3. Vérifier l'état vert de LittleFS et de l'horloge.
4. Utiliser **CSV** pour les données agrégées ou **PDF** pour le rapport lisible.

### 11. Sauvegarder ou restaurer la Centrale

Dans **Configuration**, cliquer sur **Exporter JSON** pour télécharger une
sauvegarde complète. Le fichier contient :

- la durée par attribution, la valeur monétaire des impulsions et le solde en pièces ;
- le token API ;
- tous les utilisateurs, leurs rôles et leurs hashes salés de mots de passe ;
- tous les postes configurés et leurs tarifs individuels ;
- le réseau Wi-Fi enregistré et la configuration IP.
- la préférence d'affichage des cartes d'indicateurs (`ui.statsHidden`).

Pour restaurer ces données, sélectionner le fichier JSON puis cliquer sur
**Restaurer la sauvegarde**. Les données persistantes courantes sont remplacées
et toutes les sessions web sont fermées. Il faut ensuite se reconnecter avec un
compte contenu dans la sauvegarde. Les sessions de jeu en cours, les postes
simplement découverts et le journal en RAM ne sont pas rejoués.
Les réglages Wi-Fi restaurés sont appliqués au prochain redémarrage de la
Centrale. La préférence d'affichage est appliquée immédiatement au navigateur
qui effectue la restauration. Une sauvegarde plus ancienne, sans bloc `ui`,
reste compatible et conserve la préférence actuelle du navigateur.

> Le fichier contient le token API, les identifiants Wi-Fi et les hashes des
> mots de passe. Ne pas le partager et le stocker dans un emplacement protégé.
> L'historique `/revenue.bin` de LittleFS n'est ni remplacé ni inclus par cette
> sauvegarde JSON.

### 12. Réinitialiser le Wi-Fi

Cliquer sur **Réinitialiser le Wi-Fi** et confirmer. Les identifiants Wi-Fi sont supprimés,
l'ESP32 redémarre et recrée le point d'accès de configuration.

## API HTTP

Les appels effectués par un navigateur connecté utilisent le cookie
`ESPSESSION`. Un client externe peut utiliser :

```http
Authorization: Bearer VOTRE_TOKEN_API
```

Le token API possède les permissions administrateur. Le token partagé des postes
est distinct et sert uniquement aux annonces et commandes entre ESP32.

### Routes générales

| Méthode | Route | Accès | Description |
|---|---|---|---|
| POST | `/login` | Public | Ouvre une session avec `username` et `password`. |
| POST | `/logout` | Session | Ferme la session courante. |
| GET | `/posts` | Connecté/API | État des postes et crédit disponible. |
| POST | `/assign` | Connecté/API | Ajoute une durée au tarif enregistré du poste. |
| POST | `/stop` | Connecté/API | Arrête une session. |
| POST | `/recovery/resume` | Connecté/API | Relance le temps sauvegardé. |
| POST | `/recovery/cancel` | Connecté/API | Annule le temps sauvegardé d'un poste. |

### Routes administrateur

| Méthode | Route | Description |
|---|---|---|
| POST | `/assignment/resolve` | Confirme ou annule un débit interrompu après vérification administrateur. |
| POST | `/config` | Modifie la configuration générale. |
| GET | `/config/export` | Télécharge la sauvegarde JSON complète. |
| POST | `/config/import` | Restaure directement une sauvegarde JSON complète. |
| GET | `/revenue/data?period=day|week|month&anchor=AAAA-MM-JJ` | Retourne la recette agrégée et l'état LittleFS/NTP. |
| GET | `/revenue/export.csv?period=day|week|month&anchor=AAAA-MM-JJ` | Télécharge la recette agrégée en CSV. |
| POST | `/revenue/archive/prepare` | Prépare un export intégral avec jeton temporaire (administrateur). |
| GET | `/revenue/archive/page?token=...&offset=0` | Lit l’archive par pages de 32 écritures. |
| POST | `/revenue/clear?token=...` | Avec `pdfSaved: true`, efface uniquement le journal après vérification de l’archive. |
| POST | `/post/update` | Modifie le tarif ; renomme uniquement hors session et reprise. |
| POST | `/post/delete` | Supprime un poste inactif et tente de le déconfigurer s'il est joignable. |
| POST | `/post/ping` | Teste manuellement un poste. |
| POST | `/poste/configure` | Configure un poste découvert. |
| GET | `/logs/data` | Retourne les logs. |
| POST | `/logs/clear` | Vide les logs. |
| POST | `/wifi/reset` | Supprime les identifiants Wi-Fi et redémarre. |
| POST | `/auth/password` | Change le mot de passe du compte admin courant. |
| POST | `/auth/token/regenerate` | Régénère le token API. |
| GET | `/users/data` | Liste les comptes sans les mots de passe. |
| POST | `/users/create` | Crée un compte. |
| POST | `/users/update` | Modifie un compte ou réinitialise son mot de passe. |
| POST | `/users/delete` | Supprime un compte. |

### Route réservée aux postes ESP32

| Méthode | Route | Description |
|---|---|---|
| POST | `/poste/announce` | Reçoit l'annonce d'un poste avec le token partagé. |

### Exemples

Affecter une durée au tarif enregistré du poste :

```bash
curl -X POST http://pstm.local/assign \
  -H 'Authorization: Bearer VOTRE_TOKEN_API' \
  -H 'Content-Type: application/json' \
  -d '{"post_id":"post1"}'
```

Créer un utilisateur simple :

```bash
curl -X POST http://pstm.local/users/create \
  -H 'Authorization: Bearer VOTRE_TOKEN_API' \
  -H 'Content-Type: application/json' \
  -d '{"username":"sami","firstName":"Sami","lastName":"Ben Ali","password":"motdepasse","role":"user"}'
```

## Persistance des données

La classe `Preferences` enregistre les données dans plusieurs espaces NVS :

| Namespace | Données |
|---|---|
| `wifi` | SSID, mot de passe et configuration DHCP ou IPv4 statique. |
| `appcfg` | Instantanés V4 `configV4`, `creditsV4` et `postsV4`, avec CRC ; journaux `restoreV4` et `assignmentV4`. |
| `auth` | Token API et comptes avec hash des mots de passe. |
| `posts` | Anciennes données, lues pour migration ; la liste courante est dans `appcfg/postsV4`. |
| `revenue` | Marqueur anti-formatage LittleFS et transaction financière `pending` permettant une reprise sans doublon. |

Les statuts, sessions web, postes en attente et logs restent en RAM et sont
réinitialisés au redémarrage. Les décisions de reprise ne dépendent cependant
pas de la NVS centrale : chaque poste conserve son propre instantané et le
réannonce après le redémarrage de la centrale.

Le journal financier n'est pas en NVS. Il est conservé dans `/revenue.bin` sur
LittleFS. Une écriture de 72 octets est flushée et relue après chaque lot
d'impulsions préparé durablement en NVS, avant l'instantané atomique des crédits. La partition LittleFS fait 640 Kio ; l'interface expose
son occupation afin que l'administrateur puisse la surveiller. À partir de 40 %
d'occupation, une compaction sûre regroupe automatiquement les anciens
événements compatibles par heure, session, devise et valeur. Cette granularité conserve exactement les rapports jour,
semaine et mois tout en prolongeant fortement la capacité. Un fichier temporaire
et un fichier de secours rendent la compaction récupérable après une coupure.

## Rôle de chaque fichier

| Fichier | Rôle |
|---|---|
| `central-esp32.ino` | Point d'entrée : initialisation et boucle principale. |
| `AppConfig.h` | Constantes globales, GPIO, délais, valeurs et identifiants par défaut. |
| `AppState.h` | État partagé, comptes, sessions, crédits, logs et postes en mémoire. |
| `Post.h` | Structure représentant un poste configuré, y compris son éventuelle reprise en attente. |
| `AuthService.h` | Interface du service d'authentification et de gestion des comptes. |
| `AuthService.cpp` | Hash des mots de passe, connexion, sessions, rôles et CRUD utilisateurs. |
| `CoinService.h` | Interface du gestionnaire de monnayeur. |
| `CoinService.cpp` | Interruption GPIO, anti-rebond et crédit des impulsions en pièces. |
| `RevenueService.h` | Structures et interface des rapports de recette. |
| `RevenueService.cpp` | NTP, LittleFS, journal binaire CRC16, agrégations jour/semaine/mois et CSV. |
| `LogService.h` | Interface du journal d'événements. |
| `LogService.cpp` | Journal RAM limité à 100 événements et sortie série. |
| `PostService.h` | Interface de la logique métier des postes. |
| `PostService.cpp` | Découverte, configuration, sessions, reprise après coupure, suppression et surveillance. |
| `PosteClient.h` | Interface du client HTTP destiné aux postes. |
| `PosteClient.cpp` | Envoi des commandes et lecture du statut des postes. |
| `StorageService.h` | Interface de persistance et d'import/export. |
| `StorageService.cpp` | Lecture et écriture des namespaces NVS. |
| `WifiService.h` | Interface de connexion Wi-Fi et mDNS. |
| `WifiService.cpp` | Connexion au Wi-Fi sauvegardé et publication de `pstm.local`. |
| `WifiProvisioning.h` | Interface du mode de configuration Wi-Fi. |
| `WifiProvisioning.cpp` | Point d'accès, DNS captif, scan, formulaire et sauvegarde Wi-Fi. |
| `WebRoutes.h` | Déclaration de l'enregistrement des routes web. |
| `WebRoutes.cpp` | API HTTP, contrôles d'accès et réponses JSON. |
| `WebPage.h` | Déclaration des pages HTML embarquées. |
| `WebPage.cpp` | HTML, CSS et JavaScript de la connexion et du tableau de bord. |
| `WebAssets.h` / `WebAssets.cpp` | Expose les images WebP HD intégrées au firmware. |
| `*Asset.inc` | Données binaires `PROGMEM` générées depuis les WebP embarqués. |
| `ui-assets/` | Logo maître, fond de connexion et variantes WebP HD documentées. |
| `docs/schema-cablage-central-final.svg` | Schéma vectoriel : monnayeur, GPIO13, masse commune et pull-up interne sans résistance externe vers GND. |
| `partitions.csv` | Deux partitions OTA de 1,625 Mio, 640 Kio de LittleFS et NVS conservée. |

## Sécurité

Avant une utilisation réelle :

1. changer immédiatement le mot de passe administrateur `12345678` ;
2. définir `AP_PASSWORD` dans `AppConfig.h` si le portail Wi-Fi doit être
   protégé ;
3. remplacer `POSTE_COMMAND_TOKEN` sur la centrale et tous les postes ;
4. régénérer le token API depuis l'interface ;
5. utiliser un réseau local isolé et protégé ;
6. ne pas exposer directement le port 80 à Internet.

L'interface utilise HTTP sans TLS. Les identifiants Wi-Fi sont stockés dans la
NVS sans chiffrement applicatif. Le hash salé protège les mots de passe des
comptes, mais un accès physique à la carte reste un risque à traiter au niveau du
déploiement.

## Dépannage

### Le portail Wi-Fi ne s'ouvre pas

- vérifier la connexion au SSID `GAMEROOM-CENTRAL-<chipId>` ;
- ouvrir manuellement `http://192.168.4.1` ;
- désactiver temporairement les données mobiles ou le VPN ;
- consulter le moniteur série à 115200 bauds.

### `pstm.local` ne répond pas

- utiliser l'adresse IP affichée sur le port série ;
- vérifier que le client et l'ESP32 sont sur le même réseau ;
- certains réseaux ou appareils ne prennent pas correctement en charge mDNS.

### Un poste reste hors ligne

- vérifier son alimentation et son adresse IP ;
- confirmer que les deux ESP32 utilisent le même réseau ;
- vérifier que `POSTE_COMMAND_TOKEN` est identique ;
- tester **Ping** dans la gestion des postes ;
- confirmer que le poste expose bien `GET /status`.

### Les coins ne sont pas détectés

Le monnayeur peut reconnaître une pièce sans que l'ESP32 reçoive un signal
électrique exploitable. Procéder dans cet ordre :

1. Vérifier l'accès à l'interface normale après connexion Wi-Fi. Si le moniteur
   série affiche `Monnayeur inactif`, terminer la configuration Wi-Fi.
2. Vérifier la programmation du monnayeur et le solde avant une introduction
   physique contrôlée. Chaque impulsion validée doit ajouter une pièce disponible,
   indépendamment du tarif des postes.
3. Ouvrir le moniteur série à **115200 bauds**, redémarrer et vérifier
   `Monnayeur prêt: GPIO13, rappel=pull-up interne`. Sélectionner **NO** sur le
   monnayeur, introduire une pièce puis lire `Impulsions monnayeur validées`.

| Observation | Interprétation et contrôle |
|---|---|
| Une impulsion apparaît juste après le démarrage, sans pièce | Vérifier la présence de la phase `Monnayeur en stabilisation`, puis attendre `Monnayeur prêt`. Les fronts des 5 premières secondes et les interruptions retardées avec ligne HIGH sont ignorés. |
| Le solde initial vaut déjà 1, sans nouveau log d'impulsion | Le solde chargé de NVS est conservé. Pour un essai à zéro, enregistrer `Pièces disponibles = 0` dans Configuration ; le cache du navigateur ne modifie pas la NVS. |
| Aucun log après la pièce, alors que `Monnayeur prêt` apparaît | Aucun front descendant n'a été compté. Vérifier GPIO13, le fil de sortie et la masse ; mesurer le signal. |
| Niveau initial LOW en mode NO, sans pièce | Vérifier qu'aucune résistance ne relie GPIO13 à GND et que la sortie peut remonter à HIGH au repos. |
| Deux unités sont ajoutées pour une pièce physique | Vérifier au démarrage `anti-doublon=320 ms` et la programmation physique du monnayeur. Deux fronts espacés de moins de 320 ms ne doivent produire qu'une impulsion utile. |
| Certaines impulsions d'un train rapide manquent | Le réglage par défaut vise une impulsion utile par pièce. Pour de vraies impulsions multiples programmées, adapter `COIN_DEBOUNCE_MS` à leur espacement après mesure. |
| Impulsions validées mais solde inchangé | Vérifier l’état NVS/LittleFS et les transactions en attente ; chaque impulsion finalisée ajoute une pièce. |

**Résistance de 2 kΩ à la place de 4,7 kΩ :** vers GND, elle charge environ
2,35 fois plus une sortie active à tension identique. Elle peut abaisser le
niveau haut si la sortie est faible, sans être forcément la cause. Pour un
ESP32 classique alimenté en 3,3 V, viser un niveau bas proche de 0 V et un
niveau haut proche de 3,3 V ; le seuil haut garanti est d'environ 2,48 V
([fiche Espressif, caractéristiques DC](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)).
Revenir à 4,7 kΩ ne corrige pas un mauvais type de rappel : un pull-down garde
une sortie collecteur ouvert à GND dans ses deux états. Confirmer le type de
sortie. Le firmware utilise maintenant un pull-up interne : le laisser agir
sans résistance externe de 2 kΩ ou 4,7 kΩ vers GND, qui maintiendrait l'entrée LOW.

Mesurer la sortie `COIN` **déconnectée de l'ESP32** avant tout raccordement.
Un multimètre peut manquer une impulsion brève ou en afficher la moyenne :
une lecture de 0 V ne prouve pas que le pic est sans danger. Pour connaître
l'amplitude et la durée, utiliser un oscilloscope ou une interface de signal
dont les niveaux de sortie sont garantis. Si la sortie délivre du 5 V ou du
12 V, intercaler une adaptation de niveau, par exemple un optocoupleur adapté.

Pour vérifier GPIO13 indépendamment du monnayeur, couper les alimentations,
débrancher le fil `COIN` et retirer toute résistance externe vers GND. Alimenter
ensuite l'ESP32 par USB, attendre `Monnayeur prêt` avec le niveau initial HIGH,
relier brièvement **GPIO13 à GND**
(environ 50 à 100 ms), puis retirer le fil. Le passage à LOW doit
produire `Impulsions monnayeur validées: +1` ; le
solde augmente de 1 pièce. Un contact maintenu LOW reste une seule impulsion. Cet essai
passe par le comptage réel et ajoute donc aussi une écriture de recette.

### La recette n'est pas enregistrée

- faire un téléversement USB complet pour appliquer le nouveau `partitions.csv` ;
- ouvrir **Recette** et lire l'état LittleFS/NTP affiché en haut de la page ;
- vérifier que le Wi-Fi dispose d'un accès NTP pour obtenir les dates réelles ;

### Connexion refusée après mise à jour

- utiliser le nom d'utilisateur `admin` ;
- utiliser l'ancien mot de passe administrateur, automatiquement migré ;
- sur une installation neuve, utiliser `12345678`.

## Limites connues

- les assets HD imposent le fichier `partitions.csv` personnalisé documenté ci-dessus ;
- les communications HTTP ne sont pas chiffrées ;
- les logs sont perdus au redémarrage ;
- les écritures effectuées avant NTP sont conservées mais restent non datées ;
- si LittleFS est indisponible, les crédits financiers attendent sa remise en service ; une transaction préparée reste en NVS, les impulsions suivantes restent dans une file RAM bornée ;
- la partition de 640 Kio est finie malgré la compaction horaire : surveiller son occupation dans **Recette** ;
- le temps récupérable dépend du dernier instantané écrit par le poste et peut
  être supérieur au temps réel d'environ 60 secondes avec la configuration par défaut ;
- la sauvegarde ne rejoue pas les données temporaires : journal RAM, sessions web,
  sessions de jeu actives et postes découverts non configurés ;
- les postes sont interrogés un par un : avec `N` postes, un cycle complet prend environ `N x 5` secondes ;
- la liste des postes est réécrite en NVS lors de certaines annonces, ce qui peut augmenter l'usure de la flash si les annonces sont très fréquentes ;
- le firmware compatible des postes se trouve dans le dossier voisin
  `../poste-esp32`.

## Valeurs configurables dans le code

Toutes les constantes de `AppConfig.h` utilisées par le firmware actuel sont
répertoriées ci-dessous :

| Constante | Valeur par défaut | Rôle |
|---|---|---|
| `AP_SSID_PREFIX` | `GAMEROOM-CENTRAL` | Préfixe du point d'accès de configuration. |
| `AP_PASSWORD` | chaîne vide | Mot de passe du portail ; utiliser au moins 8 caractères pour le protéger. |
| `MDNS_HOSTNAME` | `pstm` | Nom publié sous `pstm.local`. |
| `DEFAULT_ADMIN_USERNAME` | `admin` | Compte initial. |
| `DEFAULT_ADMIN_PASSWORD` | `12345678` | Mot de passe initial à changer immédiatement. |
| `POSTE_COMMAND_TOKEN` | valeur de développement | Secret partagé avec tous les postes. |
| `WIFI_CONNECT_TIMEOUT_MS` | `15000` | Délai de connexion avant ouverture du portail. |
| `SERVER_PORT` | `80` | Port HTTP de la centrale. |
| `COIN_PIN` | GPIO 13 | Entrée du monnayeur avec pull-up interne `INPUT_PULLUP`. |
| `DEFAULT_COIN_DURATION_SECONDS` | `1800` | Durée initiale par attribution. |
| `DEFAULT_PIECES_REQUIRED` | `1` | Tarif initial d’un nouveau poste. |
| `DEFAULT_PULSE_VALUE_MILLIMES` | `1000` | Valeur financière d'une impulsion. |
| `DEFAULT_AVAILABLE_PIECES` | `0` | Solde initial en pièces. |
| `REVENUE_FILE` | `/revenue.bin` | Journal financier LittleFS. |
| `REVENUE_STORAGE_RETRY_INTERVAL_MS` | `30000` | Nouvelle tentative de montage/écriture après erreur. |
| `TIMEZONE` | `CET-1` | Fuseau tunisien utilisé pour les rapports. |
| `NTP_SERVER_1` | `pool.ntp.org` | Serveur NTP principal. |
| `NTP_SERVER_2` | `time.nist.gov` | Serveur NTP secondaire. |
| `COIN_DEBOUNCE_MS` | `320` | Verrouillage anti-doublon entre fronts descendants acceptés, sans attente bloquante. |
| `COIN_STARTUP_IGNORE_MS` | `5000` | Impulsions ignorées au démarrage ; armement ensuite uniquement au repos HIGH. |
| `POST_REFRESH_INTERVAL_MS` | `5000` | Intervalle de sondage des postes. |
| `POST_OFFLINE_TIMEOUT_MS` | `10000` | Délai avant état hors ligne. |
| `POSTE_STATUS_TIMEOUT_MS` | `1200` | Timeout de lecture de `/status`. |
| `POSTE_COMMAND_TIMEOUT_MS` | `4000` | Timeout des commandes vers un poste. |
| `PENDING_POST_TIMEOUT_MS` | `30000` | Expiration d'un poste découvert non configuré. |

Après modification de ces valeurs, recompiler et téléverser le firmware.

## Nettoyage du dépôt et maintenance documentaire

Le dépôt livré contient les deux firmwares, leurs ressources, la documentation
et les deux manuels PDF. Les serveurs de démonstration, les commandes de simulation,
les données de caisse locale et les rapports PDF d’exemple ont été retirés.
Le script `docs/manuals/generate_pdfs.py` reste disponible pour reconstruire les
manuels à partir des Markdown et des illustrations dont les empreintes figurent
dans `docs/manuals/captures.json`.
