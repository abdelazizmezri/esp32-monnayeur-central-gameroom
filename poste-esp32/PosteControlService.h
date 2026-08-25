#pragma once

namespace PosteControlService {
  // Initialise le relais, l'afficheur et la tache temps reel. Si la tache ne
  // peut pas etre creee, loop() assure automatiquement le mode de secours.
  void begin();
  void loop();
}
