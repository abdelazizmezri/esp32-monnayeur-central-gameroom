# Assets de l’interface PS TIME MANAGER

- `game-room-logo-embedded.webp` : version horizontale transparente allégée
  (200 × 67 px), intégrée au firmware et servie sur
  `/assets/game-room-logo-v4.webp`.
- `ps-time-manager-logo-horizontal-hq.webp` : source maîtresse horizontale haute
  qualité avec fond transparent (2172 × 724 px), convertie sans perte et adaptée
  aux en-têtes comme à l’impression.
- `login-station-source.webp` : image source 1123 × 1401 px de la station de jeu,
  convertie sans perte et utilisée pour produire le panneau gauche optimisé.
- `login-station-embedded.webp` : version optimisée (400 × 499 px, qualité 35)
  intégrée au firmware et servie sur `/assets/login-station-v3.webp`. Elle
  remplace l’ancienne version JPG 192 × 240 px afin d’améliorer la netteté sans
  changer le schéma de partitions.
- `playstation-mark.webp` : icône PlayStation multicolore transparente allégée
  (80 × 61 px), utilisée pour identifier les nouveaux postes détectés avant
  leur configuration.

Les fichiers `LoginStationAsset.inc`, `GameRoomLogoAsset.inc` et
`PlayStationMarkAsset.inc` sont les représentations `PROGMEM` générées depuis
ces versions WebP allégées. Les sources haute définition sont encodées sans
perte, tandis que l’image embarquée de connexion utilise une compression avec
perte maîtrisée. Cette optimisation permet de conserver le schéma de partitions
ESP32 4 Mo par défaut sans stockage externe.
