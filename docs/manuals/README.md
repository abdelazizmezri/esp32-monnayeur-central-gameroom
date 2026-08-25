# Documentation illustrée

Ce dossier contient les sources utilisées pour produire les deux documents de
livraison :

- `mock_server.py` extrait les pages HTML embarquées dans le firmware et expose
  une API locale entièrement fictive pour les captures ;
- `generate_pdfs.py` compose le guide technique client et le manuel
  d'utilisation à partir du code, des schémas SVG et des captures validées.

Les fichiers livrés sont placés dans `output/pdf/` et les captures sources dans
`output/screenshots/`.

La version finale 1.1 du 25 août 2026 comprend :

- un guide technique client de 27 pages : architecture, sécurité électrique,
  nomenclature, Arduino IDE 2.3.10, `arduino-cli` 1.5.1, cœur ESP32 3.3.11,
  ArduinoJson 7.4.3, secrets et GPIO, table `partitions.csv` réelle de la
  centrale, réglages des deux firmwares, téléversement USB, schémas de câblage
  dont le raccordement minimal du monnayeur avec pull-down 4,7 kΩ,
  Wi-Fi, postes, comptes, sécurité, recette et procès-verbal de mise en service ;
- un manuel d'utilisation de 21 pages : rôles, première configuration,
  tableaux de bord administrateur et utilisateur simple, sessions, recette,
  sauvegarde/restauration, postes, utilisateurs, sécurité, journal, Wi-Fi,
  routines quotidiennes, dépannage et checklist de remise à un nouveau client.

## Protection des données

Le serveur documentaire n'interroge ni les ESP32 ni leur mémoire NVS. Il génère
quatre postes de démonstration couvrant les états actif, inactif, hors ligne et
reprise en attente, ainsi qu'une recette, des comptes, des événements, des SSID
et des adresses IP fictifs. Les mots de passe ne sont jamais affichés et le token
API reste masqué.

## Reproduction locale

Depuis la racine du dépôt :

```bash
python3 docs/manuals/mock_server.py
```

Ouvrir ensuite `http://127.0.0.1:8765/login` pour contrôler ou renouveler les
captures, puis générer les PDF avec :

```bash
python3 docs/manuals/generate_pdfs.py
```

La génération nécessite Python 3, ReportLab et Pillow. Les PDF doivent être
rendus en images après chaque modification afin de vérifier visuellement toutes
les pages, notamment les schémas et les captures d'écran.
