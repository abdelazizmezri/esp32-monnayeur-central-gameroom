# Assets de l’interface PS TIME MANAGER

- `game-room-logo.png` : logo horizontal officiel PS TIME MANAGER, conservé en pleine définition (nom de fichier historique utilisé par l’application).
- `game-room-logo-embedded.webp` : version horizontale transparente en haute définition, intégrée au firmware et servie sur `/assets/game-room-logo-v4.webp`.
- `ps-time-manager-logo-vertical-hq.png` : version verticale officielle en PNG haute qualité (1536 × 1024 px).
- `ps-time-manager-logo-horizontal-hq.png` : déclinaison horizontale haute qualité avec fond transparent (2172 × 724 px), adaptée aux en-têtes et à l’impression.
- `login-station.png` : image source de la station de jeu utilisée dans le panneau gauche.
- `login-station-embedded.jpg` : version optimisée intégrée au firmware et servie sur `/assets/login-station-v2.jpg`, recompressée afin de réserver davantage d’espace au logo transparent.
- `playstation-mark.png` : icône PlayStation multicolore transparente utilisée pour identifier les nouveaux postes détectés avant leur configuration.

La maquette de référence validée est archivée dans `../docs/ui/login-reference.png`. Les fichiers `LoginStationAsset.inc`, `GameRoomLogoAsset.inc` et `PlayStationMarkAsset.inc` sont les représentations `PROGMEM` générées depuis les versions optimisées afin de servir les images sans stockage externe.
