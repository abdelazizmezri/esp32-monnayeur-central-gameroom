# Assets de l’interface GAME ROOM

- `game-room-logo.png` : logo officiel fourni, conservé en pleine définition.
- `game-room-logo-embedded.jpg` : version optimisée intégrée au firmware et servie sur `/assets/game-room-logo-v2.jpg`.
- `login-station.png` : image source de la station de jeu utilisée dans le panneau gauche.
- `login-station-embedded.jpg` : version optimisée intégrée au firmware et servie sur `/assets/login-station-v2.jpg`.

La maquette de référence validée est archivée dans `../docs/ui/login-reference.png`. Les fichiers `LoginStationAsset.inc` et `GameRoomLogoAsset.inc` sont les représentations `PROGMEM` générées depuis les versions optimisées afin de servir les images sans stockage externe.
