# Documentation et manuels PS TIME MANAGER

Les deux manuels correspondent à l’interface actuelle : devises par session de
caisse, clôture, sélection de l’historique, exports PDF/CSV, archivage complet et
vidage du seul journal de recette. Les valeurs DZD par impulsion incluent
5, 10, 20, 50, 100, 200 et 500 DA.

| Source | PDF livré | Public |
|---|---|---|
| [Manuel d’utilisation](manuel-utilisation.md) | [Manuel d’utilisation](../../output/pdf/ps-time-manager-manuel-utilisation.pdf) | Administrateurs et opérateurs |
| [Guide technique](guide-technique.md) | [Guide technique client](../../output/pdf/ps-time-manager-guide-technique-client.pdf) | Installation et maintenance |

## Reconstruire les PDF

Conserver `generate_pdfs.py`, les deux sources Markdown et les illustrations
référencées. Installer Python 3.10 ou supérieur et les bibliothèques nécessaires :

```sh
python3 -m pip install reportlab Pillow pypdf
python3 docs/manuals/generate_pdfs.py
```

Les deux PDF sont écrits dans `output/pdf/`. Le script contrôle les empreintes
du [manifeste](captures.json), les images et les liens locaux avant génération.
Les PDF comportent un sommaire cliquable et des signets de navigation.

## Illustrations conservées

Les écrans de connexion, d’accueil, de gestion des postes, de configuration et
de recette ont été renouvelés le 28 octobre 2026. Les illustrations inchangées
des comptes, du réseau et du matériel ont été conservées. Leur date d’origine,
leurs dimensions et leurs empreintes restent indiquées dans le manifeste.
Les captures sont des rendus réels du HTML embarqué avec des données fictives,
sans retouche des images. L’ajout de 500 DA le 30 octobre 2026 ne modifie pas
les captures de configuration, dont le sélecteur de valeur est fermé ; leurs
empreintes de source à la capture restent conservées. Elles ne valident pas les signaux GPIO, les relais,
le TM1637, la radio ou les coupures d’alimentation d’un appareil.

Les images inutilisées, les anciens écrans de change, les rapports PDF d’exemple,
les jeux de données et les programmes de simulation ont été retirés du dépôt.
La compilation, le téléversement et la vérification physique s’effectuent avec
les outils Arduino décrits dans le guide technique.

## Notices complémentaires

- [Caisse et historique de recette](../multidevise.md)
- [Tarification par poste et migration V4](../tarification-par-poste.md)
- [Centrale ESP32](../../central-esp32/README.md)
- [Postes ESP32](../../poste-esp32/README.md)

| Illustration | Page |
|---|---|
| [Comptes : formulaire d’ajout et liste des rôles](../../output/screenshots/07-utilisateurs.jpg) | `/users` |
| [Sécurité : double saisie, force du mot de passe, token et indications](../../output/screenshots/08-securite.jpg) | `/security` |
| [Journal : niveaux, messages, horodatage relatif et commandes](../../output/screenshots/09-journal.jpg) | `/logs` |
| [Portail Wi-Fi de la centrale : sélection du réseau et DHCP](../../output/screenshots/10-wifi-centrale.jpg) | `/wifi` |
| [Portail poste : mêmes réglages réseau, sans gestion locale du solde](../../output/screenshots/11-wifi-poste.jpg) | `/wifi` |
| [Éditeur utilisateur : identité, rôle et mot de passe facultatif](../../output/screenshots/16-modifier-utilisateur.jpg) | `/users` |
| [Centrale en IP fixe : adresse, passerelle, masque et DNS](../../output/screenshots/19-wifi-centrale-ip-fixe.jpg) | `/wifi` |
| [Portail du poste : paramètres IP fixes dépliés](../../output/screenshots/20-wifi-poste-ip-fixe.jpg) | `/wifi` |
| [Réponse locale du poste en mode normal : Poste OK](../../output/screenshots/21-poste-local.jpg) | `/` |
| [Réponse JSON du poste : identité, relais, temps et reprise](../../output/screenshots/22-poste-status.jpg) | `/status` |
| [Menu mobile ouvert : navigation et commandes administrateur](../../output/screenshots/25-menu-mobile.jpg) | `/` |
| [Portail Wi-Fi du poste sur mobile : formulaire après défilement](../../output/screenshots/27-wifi-poste-mobile.jpg) | `/wifi` |
| [Accueil](../../output/screenshots/interface-accueil.png) | `/` |
| [Configuration dzd](../../output/screenshots/interface-configuration-dzd.png) | `/config` |
| [Configuration](../../output/screenshots/interface-configuration.png) | `/config` |
| [Connexion](../../output/screenshots/interface-connexion.png) | `/login` |
| [Memoire recette](../../output/screenshots/interface-memoire-recette.png) | `/recette` |
| [Postes](../../output/screenshots/interface-postes.png) | `/postes` |
| [Recette](../../output/screenshots/interface-recette.png) | `/recette` |
| [Schéma actuel : GPIO13 avec pull-up interne, masse commune et alimentation séparée](../../output/screenshots/schema-cablage-centrale.jpg) | `None` |
| [Schéma du poste : relais GPIO27, TM1637 CLK25 et DIO26](../../output/screenshots/schema-cablage-poste.jpg) | `None` |
