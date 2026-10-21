# Documentation illustrée - centrale et postes

Sources des guides destinés à l’installateur et aux utilisateurs de PS TIME MANAGER.

## Documents livrés

| Source française | PDF généré | Contenu |
|---|---|---|
| [Guide technique](guide-technique.md) | [Guide technique client](../../output/pdf/ps-time-manager-guide-technique-client.pdf) | Installation pas à pas d’Arduino IDE, ESP32 et ArduinoJson ; configuration, câblage, réseau et dépannage |
| [Manuel d’utilisation](manuel-utilisation.md) | [Manuel d’utilisation](../../output/pdf/ps-time-manager-manuel-utilisation.pdf) | Procédures par rôle : prérequis, champs, boutons, confirmations et résultats attendus |

Les README [général](../../README.md), [centrale](../../central-esp32/README.md)
et [poste](../../poste-esp32/README.md) complètent ces guides avec les détails
des firmwares. Les notices [multidevise](../multidevise.md) et
[tarification V4](../tarification-par-poste.md) détaillent les migrations.

## Provenance et limites des captures

Les **29 captures d’application** et les **2 rendus de schémas** sont des fichiers
JPEG dans [`output/screenshots/`](../../output/screenshots/). Le
[manifeste de provenance](captures.json) donne leurs dimensions, empreintes SHA-256,
routes documentaires et firmware, rôles et empreintes des sources de l’interface.
Les captures ne sont pas des maquettes et ne sont pas retouchées.

Le serveur `mock_server.py` extrait le HTML/CSS/JavaScript des pages embarquées
et reconstruit les blocs réseau avec des SSID fictifs. Il sert les WebP du dépôt.
Il ne lit ni les ESP32, ni leur NVS, ni leur réseau Wi-Fi. Les postes, comptes,
IP, recettes, logs et le taux 0,020000 sont des exemples fictifs. Le statut JSON
et la réponse « Poste OK » reproduisent les réponses décrites dans le code
du poste ; leurs URL locales documentaires ne sont pas des routes ajoutées au
firmware.

L’API documentaire conserve quelques actions en RAM : attribution, arrêt/reprise,
paramètres, simulation et tarif. Les autres écritures renvoient 501 ; elles ne
prétendent pas avoir réussi. La connexion choisit le rôle utilisateur pour
`user-demo`, et le rôle administrateur pour les autres identifiants ; elle ne
valide pas de vrais mots de passe. L’export JSON documentaire est incomplet et
marqué `documentationOnly` : il ne faut jamais le restaurer sur une centrale.
Le fichier `{}` de la capture 28 démontre seulement la sélection et la validation
syntaxique du JSON par le navigateur, sans import.

Ne sont pas validés : compilation actuelle, téléversement, droits du serveur
réel, connexions radio et pages de succès/échec Wi-Fi, impulsions du monnayeur,
relais, afficheur TM1637, écritures NVS/LittleFS, restauration et coupures.
Le panneau conditionnel d’un débit interrompu est décrit depuis le code.
Les confirmations natives du navigateur ne figurent pas dans les captures.
Les captures mobiles montrent une fenêtre de 390 × 844 pixels ; faire défiler
pour accéder aux différentes parties. Aucun firmware n’a été corrigé pour les captures.

## Reproduire l’aperçu et les captures

Depuis la racine du dépôt, Python 3.10 ou supérieur :

```sh
python3 docs/manuals/mock_server.py
```

Ouvrir `http://127.0.0.1:8765/login`. Les liens ci-dessous utilisent les alias
locaux du serveur documentaire. Pour changer de rôle explicitement, utiliser
`http://127.0.0.1:8765/?role=admin` ou `http://127.0.0.1:8765/?role=user`.
Sur les ESP32, utiliser les chemins firmware du manifeste, notamment `/wifi`
sur chaque appareil ; les alias `/wifi-central`, `/wifi-poste` et `/poste-local`
n’existent pas sur les cartes.

Parcourir les pages et activer les champs et boutons nécessaires. Capturer le
rendu réel en JPEG, sans injecter de faux éléments dans le DOM. Les vues mobiles
ont été capturées à 390 × 844 pixels, avant ou après défilement. Les pages
longues de recette sont capturées intégralement. Pour les portails et les
réponses texte, le cadrage direct se limite à la zone utile de la fenêtre ;
il ne modifie ni le HTML ni les proportions.
Recréer les métadonnées de provenance après renouvellement d’une image. Les
interactions locales changent la RAM du serveur ; le redémarrer pour rétablir
les quatre états initiaux et le solde de 18 pièces.

## Régénérer les PDF

Installer ReportLab, Pillow et pypdf dans l’environnement Python utilisé :

```sh
python3 -m pip install reportlab Pillow pypdf
python3 docs/manuals/generate_pdfs.py
```

Les sources faisant foi sont `guide-technique.md` et `manuel-utilisation.md`.
Le générateur utilise les polices Vera livrées avec ReportLab, vérifie les
liens locaux et les empreintes de provenance, puis intègre les captures sans
déformation. Les images longues
sont réparties en bandes successives à largeur conservée ; aucune fonctionnalité
ni donnée n’est ajoutée dans l’image. Les titres sont accessibles par signets PDF.

Après chaque modification, extraire le texte pour vérifier les références et l’absence de dates éditoriales,
puis rendre toutes les pages et les inspecter :

```sh
pdftoppm -r 110 -png output/pdf/ps-time-manager-guide-technique-client.pdf /tmp/pstm-technique
pdftoppm -r 110 -png output/pdf/ps-time-manager-manuel-utilisation.pdf /tmp/pstm-utilisation
```

Les rendus de contrôle sont temporaires et ne font pas partie de la livraison.

## Présentation des PDF

Le guide technique suit la préparation de l’ordinateur, l’installation des outils,
la programmation des deux firmwares, le câblage et la mise en réseau. Le manuel
d’utilisation distingue les tâches communes aux deux rôles et celles de
l’administrateur. Les étapes numérotées sont rendues comme de vraies listes.

Les PDF ne contiennent aucune date éditoriale ni commentaire sur les travaux
de documentation. Les métadonnées CreationDate/ModDate sont retirées. Les dates
des exemples de recette restent dans les captures : elles ont une fonction
comptable et ne sont pas des dates de publication. Les notes de provenance et
les instructions de génération restent dans ce dossier, hors des PDF client.

## Inventaire des écrans

| Capture | Écran ou élément couvert | Route documentaire |
|---|---|---|
| [01-connexion.jpg](../../output/screenshots/01-connexion.jpg) | Connexion : identifiant, mot de passe, affichage et mémorisation | `/login` |
| [02-tableau-de-bord-admin.jpg](../../output/screenshots/02-tableau-de-bord-admin.jpg) | Centrale : vue des postes avec cinq indicateurs et quatre états | `/` |
| [03-tableau-de-bord-utilisateur.jpg](../../output/screenshots/03-tableau-de-bord-utilisateur.jpg) | Vue utilisateur simple : sessions accessibles sans les modules administrateur | `/` |
| [04-recette.jpg](../../output/screenshots/04-recette.jpg) | Vue Jour : filtres, état de stockage, synthèse, graphique et détail | `/recette` |
| [05-configuration.jpg](../../output/screenshots/05-configuration.jpg) | Configuration : durée, devise, taux, solde et sauvegarde | `/config` |
| [06-gestion-des-postes.jpg](../../output/screenshots/06-gestion-des-postes.jpg) | Gestion des postes : découverte, nom, tarif et commandes de gestion | `/postes` |
| [07-utilisateurs.jpg](../../output/screenshots/07-utilisateurs.jpg) | Comptes : formulaire d’ajout et liste des rôles | `/users` |
| [08-securite.jpg](../../output/screenshots/08-securite.jpg) | Sécurité : double saisie, force du mot de passe, token et indications | `/security` |
| [09-journal.jpg](../../output/screenshots/09-journal.jpg) | Journal : niveaux, messages, horodatage relatif et commandes | `/logs` |
| [10-wifi-centrale.jpg](../../output/screenshots/10-wifi-centrale.jpg) | Portail Wi-Fi de la centrale : sélection du réseau et DHCP | `/wifi-central` |
| [11-wifi-poste.jpg](../../output/screenshots/11-wifi-poste.jpg) | Portail poste : mêmes réglages réseau, sans gestion locale du solde | `/wifi-poste` |
| [12-cartes-masquees.jpg](../../output/screenshots/12-cartes-masquees.jpg) | Indicateurs masqués : la liste des postes reste disponible | `/` |
| [13-modifier-tarif-poste.jpg](../../output/screenshots/13-modifier-tarif-poste.jpg) | Éditeur de poste : nom et nombre de pièces requis | `/postes` |
| [14-configuration-dzd.jpg](../../output/screenshots/14-configuration-dzd.jpg) | Formulaire DZD : valeur par impulsion et taux manuel | `/config` |
| [15-validation-configuration.jpg](../../output/screenshots/15-validation-configuration.jpg) | Validation réelle du formulaire : un taux nul est refusé avant envoi | `/config` |
| [16-modifier-utilisateur.jpg](../../output/screenshots/16-modifier-utilisateur.jpg) | Éditeur utilisateur : identité, rôle et mot de passe facultatif | `/users` |
| [17-recette-semaine.jpg](../../output/screenshots/17-recette-semaine.jpg) | Vue Semaine : synthèse et détail journalier | `/recette` |
| [18-recette-mois.jpg](../../output/screenshots/18-recette-mois.jpg) | Vue Mois : graphique et tableau pour tout le mois de référence | `/recette` |
| [19-wifi-centrale-ip-fixe.jpg](../../output/screenshots/19-wifi-centrale-ip-fixe.jpg) | Centrale en IP fixe : adresse, passerelle, masque et DNS | `/wifi-central` |
| [20-wifi-poste-ip-fixe.jpg](../../output/screenshots/20-wifi-poste-ip-fixe.jpg) | Portail du poste : paramètres IP fixes dépliés | `/wifi-poste` |
| [21-poste-local.jpg](../../output/screenshots/21-poste-local.jpg) | Réponse locale du poste en mode normal : Poste OK | `/poste-local` |
| [22-poste-status.jpg](../../output/screenshots/22-poste-status.jpg) | Réponse JSON du poste : identité, relais, temps et reprise | `/status` |
| [23-session-demarree.jpg](../../output/screenshots/23-session-demarree.jpg) | Après attribution : 14 pièces disponibles et Poste Démo 2 actif pour 30 minutes | `/` |
| [24-accueil-mobile.jpg](../../output/screenshots/24-accueil-mobile.jpg) | Accueil mobile : en-tête, bouton de menu et indicateurs | `/` |
| [25-menu-mobile.jpg](../../output/screenshots/25-menu-mobile.jpg) | Menu mobile ouvert : navigation et commandes administrateur | `/` |
| [26-configuration-mobile.jpg](../../output/screenshots/26-configuration-mobile.jpg) | Configuration mobile : première partie du formulaire | `/config` |
| [27-wifi-poste-mobile.jpg](../../output/screenshots/27-wifi-poste-mobile.jpg) | Portail Wi-Fi du poste sur mobile : formulaire après défilement | `/wifi-poste` |
| [28-sauvegarde-selectionnee.jpg](../../output/screenshots/28-sauvegarde-selectionnee.jpg) | Sélection d’un JSON : nom du fichier et bouton de restauration activé | `/config` |
| [29-widgets-recette.jpg](../../output/screenshots/29-widgets-recette.jpg) | Six widgets de recette : montants natifs, total TND et compteurs | `/recette` |
| [schema-cablage-centrale.jpg](../../output/screenshots/schema-cablage-centrale.jpg) | Schéma actuel : GPIO13 avec pull-up interne, masse commune et alimentation séparée | `/schema-centrale` |
| [schema-cablage-poste.jpg](../../output/screenshots/schema-cablage-poste.jpg) | Schéma du poste : relais GPIO27, TM1637 CLK25 et DIO26 | `/schema-poste` |
