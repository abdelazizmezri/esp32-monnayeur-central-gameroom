# Assets de l’interface PS TIME MANAGER

- `login-station-embedded.webp` : photographie HD 1123 × 1401 px du panneau
  gauche de la page de connexion. Elle est encodée en WebP qualité 88 à partir
  de l’image de référence et servie sur `/assets/login-station-v4.webp`.
- `ps-time-manager-logo-horizontal-hq.webp` : source maîtresse transparente
  2172 × 724 px du logo PS TIME MANAGER.
- `game-room-logo-embedded.webp` : déclinaison transparente 720 × 240 px du
  logo maître, encodée en WebP qualité 92 et servie sur
  `/assets/game-room-logo-v5.webp`. Sa définition couvre le panneau de
  connexion et la barre latérale sur les écrans à forte densité.
- `playstation-mark.webp` : pictogramme PlayStation transparent 128 × 97 px,
  restauré depuis la source d’origine et encodé en WebP sans perte. Il est
  servi sur `/assets/playstation-mark-v2.webp`.

Les autres pictogrammes de la connexion, de la navigation, des cartes et des
actions sont des SVG intégrés au HTML. Ils restent donc nets quelle que soit la
résolution de l’écran et ne nécessitent aucune variante bitmap.

Les fichiers `LoginStationAsset.inc`, `GameRoomLogoAsset.inc` et
`PlayStationMarkAsset.inc` sont les représentations `PROGMEM` générées depuis
les trois WebP embarqués. Le firmware central utilise le fichier
`../partitions.csv` : deux partitions OTA de 1,625 Mio, 640 Kio de LittleFS et
conservation de la NVS.

## Documentation illustrée

Les captures de l’interface sont conservées dans
[`output/screenshots/`](../../output/screenshots/) et non dans ce dossier.
Le [manifeste](../../docs/manuals/captures.json) relie chaque image à sa page
d'origine et consigne ses dimensions et son empreinte. Les fichiers WebP de ce
dossier servent de sources aux assets embarqués ; les fichiers `*Asset.inc`
sont inclus dans le firmware.
