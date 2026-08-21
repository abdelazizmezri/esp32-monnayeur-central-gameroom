# Assets de l’interface PS TIME MANAGER

- `game-room-logo-embedded.webp` : version horizontale transparente allégée
  (200 × 67 px), intégrée au firmware et servie sur
  `/assets/game-room-logo-v4.webp`.
- `ps-time-manager-logo-vertical-hq.png` : version verticale officielle en PNG haute qualité (1536 × 1024 px).
- `ps-time-manager-logo-horizontal-hq.png` : source maîtresse horizontale haute
  qualité avec fond transparent (2172 × 724 px), adaptée aux en-têtes et à
  l’impression.
- `login-station.png` : image source de la station de jeu utilisée dans le panneau gauche.
- `login-station-embedded.jpg` : version optimisée (192 × 240 px) intégrée au
  firmware et servie sur `/assets/login-station-v2.jpg`.
- `playstation-mark.png` : icône PlayStation multicolore transparente allégée
  (80 × 61 px), utilisée pour identifier les nouveaux postes détectés avant
  leur configuration.

Les fichiers `LoginStationAsset.inc`, `GameRoomLogoAsset.inc` et
`PlayStationMarkAsset.inc` sont les représentations `PROGMEM` générées depuis
ces versions allégées. Cette optimisation permet de conserver le schéma de
partitions ESP32 4 Mo par défaut sans stockage externe.
