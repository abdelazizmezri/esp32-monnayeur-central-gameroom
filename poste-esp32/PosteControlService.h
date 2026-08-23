#pragma once

namespace PosteControlService {
  // Initialise le relais, l'afficheur et la tache temps reel. Retourne false si
  // la tache n'a pas pu etre creee ; loop() assure alors le mode de secours.
  bool begin();
  void loop();
}
