# Manuel d’utilisation


Utilisez PS TIME MANAGER pour suivre les postes de jeu, attribuer du temps et administrer la salle. Ce manuel distingue les tâches de l’utilisateur simple et celles de l’administrateur. Les postes, réseaux, comptes et montants illustrés sont des exemples ; utilisez les valeurs de votre installation.

## 1. Choisir le parcours correspondant à son rôle

L’utilisateur simple exploite les sessions depuis **Vue des postes**. L’administrateur dispose de ces mêmes fonctions et des modules **Recette**, **Gestion des postes**, **Utilisateurs**, **Sécurité**, **Journal** et **Configuration**.

| Fonction | Utilisateur simple | Administrateur |
|---|---|---|
| Consulter postes, solde, tarifs et temps | Oui | Oui |
| Démarrer, prolonger ou arrêter une session | Oui | Oui |
| Reprendre ou annuler un temps sauvegardé | Oui | Oui |
| Afficher ou masquer les indicateurs | Oui | Oui |
| Corriger le solde et régler la durée | Non | Oui |
| Ajouter, modifier et supprimer des postes ou des comptes | Non | Oui |
| Consulter, exporter, clôturer et vider la recette | Non | Oui |
| Sauvegarde, restauration, sécurité, journal et Wi-Fi centrale | Non | Oui |
| Résoudre un débit interrompu | Non | Oui |

**Pour commencer :** les sections de connexion, de sessions, d’indicateurs, de navigation mobile et de déconnexion concernent les deux rôles. Les procédures marquées **Administrateur** complètent ce parcours. Pour installer les cartes et les logiciels, consulter le [guide technique](guide-technique.md).

## 2. Se connecter à la centrale

**Accès :** utilisateur simple et administrateur. **Prérequis :** être sur le réseau local et disposer d’un compte.

1. Ouvrir `http://pstm.local` ou l’adresse IP de la centrale.
2. Saisir **Nom d’utilisateur** et **Mot de passe**.
3. Utiliser le bouton d’affichage du mot de passe si nécessaire pour vérifier la saisie.
4. Choisir si **Se souvenir de moi** doit rester coché sur cet appareil, puis cliquer sur **Se connecter**.
5. Si la connexion est refusée, corriger la saisie. Un compte utilisateur simple doit demander à un administrateur la réinitialisation de son mot de passe si nécessaire.

**Résultat attendu :** **Vue des postes** s’ouvre et le compte courant apparaît dans le menu. L’administrateur voit les modules de gestion ; l’utilisateur simple voit son parcours d’exploitation.

Sur une installation neuve non personnalisée, le compte initial est `admin` / `12345678`. Le mot de passe doit ensuite être changé par l’administrateur. **Se souvenir de moi** conserve le cookie pendant 30 jours, mais un redémarrage de la centrale impose une nouvelle connexion car les sessions sont en RAM.

![Connexion : identifiant, mot de passe et mémorisation](../../output/screenshots/interface-connexion.png)

## 3. Consulter les postes et les indicateurs

**Accès :** utilisateur simple et administrateur. **Prérequis :** session ouverte sur la centrale.

1. Ouvrir **Vue des postes** dans le menu.
2. Lire les cinq indicateurs : **Pièces disponibles**, **Durée par attribution**, **Postes actifs**, **Postes hors ligne** et **Reprises en attente**.
3. Repérer la carte du poste voulu par son nom et, si nécessaire, son IP.
4. Lire son **Tarif**, son état, son temps restant et l’indication **Relais ON/OFF**.
5. Avant toute commande, vérifier que le poste choisi est le bon. Attendre l’actualisation des valeurs si son état vient de changer.

**Résultat attendu :** vous connaissez le solde partagé, le coût d’une attribution et l’état de chaque poste. Un poste **Hors ligne** est indisponible ; **Reprise en attente** demande une décision avant une nouvelle attribution. L’état général de l’en-tête ne remplace pas l’état individuel des postes.

![Administrateur : mêmes cartes de session et modules de gestion](../../output/screenshots/interface-accueil.png)

## 4. Masquer ou réafficher les indicateurs

**Accès :** utilisateur simple et administrateur. **Emplacement :** **Vue des postes**.

1. Cliquer sur **Masquer les cartes** pour replier les cinq indicateurs.
2. Continuer à consulter et commander les postes dans la liste affichée.
3. Cliquer sur **Afficher les cartes** pour rétablir les indicateurs.

**Résultat attendu :** seule leur visibilité change. Le solde et les sessions ne sont pas modifiés. Le choix est mémorisé dans le navigateur utilisé.


## 5. Démarrer une session de jeu

**Accès :** utilisateur simple et administrateur. **Prérequis :** poste inactif et joignable, solde suffisant, aucune reprise à résoudre et stockage disponible.

1. Ouvrir **Vue des postes** et choisir le poste du client.
2. Lire son tarif et la durée commune. Le bouton **+N** consomme N pièces pour ajouter une seule durée de jeu.
3. Vérifier que le solde contient au moins N pièces.
4. Cliquer une fois sur **+N** et attendre la réponse avant une nouvelle action.
5. Vérifier que la carte devient **Actif**, que le relais est affiché ON et que le temps apparaît. Contrôler le poste et son afficheur si nécessaire.

**Résultat attendu :** le poste démarre et le solde est diminué du tarif. Exemple : avec 18 pièces disponibles, un clic sur **+4** laisse 14 pièces et ajoute 30 minutes si la durée commune vaut 30 minutes.

Si le bouton est désactivé, lire l’explication de la carte : solde insuffisant, poste indisponible, reprise non résolue ou problème de stockage. Après un timeout, vérifier si le poste a reçu le temps avant de recommencer.


## 6. Prolonger une session active

**Accès :** utilisateur simple et administrateur. **Prérequis :** poste actif et joignable, pièces suffisantes pour son tarif.

1. Sur **Vue des postes**, vérifier le nom et le temps restant du poste actif.
2. Lire le tarif courant : il peut avoir été modifié par l’administrateur depuis le début de la session.
3. Cliquer une fois sur **+N**.
4. Attendre la réponse, puis vérifier l’augmentation du temps restant et la diminution du solde de N pièces.

**Résultat attendu :** une durée commune s’ajoute au temps de la session existante. Le tarif détermine les pièces consommées ; il ne multiplie pas la durée ajoutée. Ne pas enchaîner les clics pour compenser une réponse réseau incertaine.

## 7. Arrêter une session avant son terme

**Accès :** utilisateur simple et administrateur. **Prérequis :** session active, poste joignable et accord pour perdre le temps restant.

1. Dans **Vue des postes**, repérer la carte du poste concerné.
2. Cliquer sur **Arrêter la session**.
3. Lire la confirmation, qui précise que le temps restant sera définitivement perdu.
4. Confirmer pour arrêter, ou annuler la boîte de dialogue pour conserver la session.
5. Après confirmation, vérifier le temps à zéro et le relais OFF. En cas d’erreur réseau, vérifier le poste avant une nouvelle commande.

**Résultat attendu :** la session est interrompue. Les pièces déjà dépensées ne sont pas remboursées. À expiration normale du temps, le poste coupe aussi son relais automatiquement, sans navigateur ouvert.

## 8. Reprendre ou annuler un temps sauvegardé

**Accès :** utilisateur simple et administrateur. **Prérequis :** carte **Reprise en attente** après redémarrage d’un poste.

### Reprendre le temps

1. Repérer la carte marquée **Reprise en attente** et lire le temps récupérable.
2. Vérifier l’identité du poste et la situation du client. Le relais reste OFF dans l’attente d’une décision.
3. Cliquer sur **Reprendre**, puis confirmer la reprise proposée.
4. Vérifier le passage à **Actif**, le relais ON et le temps repris.

**Résultat attendu :** le temps sauvegardé est relancé, sans achat d’une nouvelle durée. Pour ajouter ensuite du temps, utiliser **+N** au tarif normal.

### Annuler le temps

1. Sur la carte concernée, cliquer sur **Annuler**.
2. Lire puis confirmer l’annulation définitive, ou fermer la confirmation pour garder le temps sauvegardé.
3. Vérifier que le poste revient au repos, temps à zéro et relais OFF.

**Résultat attendu :** le temps récupérable est effacé. Le temps hors tension n’est pas déduit de la reprise ; la sauvegarde périodique peut restituer environ une minute de plus que le dernier temps réel.

## 9. Se déconnecter et changer d’opérateur

**Accès :** utilisateur simple et administrateur. **Prérequis :** session web ouverte.

1. Ouvrir le menu si vous êtes sur mobile.
2. Cliquer sur **Déconnexion** dans la partie basse de la barre latérale.
3. Vérifier le retour à la page **Connexion**.
4. Pour changer d’opérateur, se connecter avec le compte de la personne qui reprend le service.

**Résultat attendu :** la session web de cet opérateur est fermée. La déconnexion ne coupe pas les sessions de jeu en cours ; les postes continuent leur décompte local.

## 10. Utiliser l’interface sur téléphone

**Accès :** utilisateur simple et administrateur, selon les droits du compte. **Prérequis :** téléphone connecté au LAN de la centrale.

1. Ouvrir l’IP de la centrale ou `http://pstm.local` et se connecter.
2. Utiliser **Ouvrir le menu** dans l’en-tête pour afficher la navigation.
3. Choisir le module autorisé : **Vue des postes** pour l’exploitation, et les modules supplémentaires pour l’administrateur.
4. Faire défiler pour atteindre les cartes de poste ou la suite d’un formulaire.
5. Effectuer la même procédure que sur ordinateur, puis lire le résultat avant de quitter la page.

**Résultat attendu :** les fonctions sont accessibles dans une disposition adaptée à la largeur de l’écran. Les illustrations suivantes montrent un compte administrateur ; le menu utilisateur simple contient moins d’options.


![Menu mobile administrateur ouvert](../../output/screenshots/25-menu-mobile.jpg)


## 11. Administrateur : ajouter un poste découvert

**Prérequis :** poste allumé, connecté au même LAN et configuré avec le secret commun de la centrale.

1. Ouvrir **Gestion des postes**.
2. Dans **Postes découverts**, repérer le poste par son IP et son identifiant matériel. **Actualiser** permet de relire la liste.
3. Saisir **Nom du poste**, par exemple `PS2`.
4. Saisir **Nombre de pièces requis**, entier de 1 à 2 147 483 647. La valeur proposée pour un nouveau poste est 1.
5. Cliquer sur **Ajouter ce poste** et lire le message de résultat.
6. Vérifier sa présence dans **Postes configurés**, puis dans **Vue des postes**.

**Résultat attendu :** le poste possède un nom et un tarif persistants. L’identifiant matériel est repris automatiquement ; il n’est pas à saisir. Une découverte non configurée disparaît après 30 secondes sans annonce ; vérifier le réseau si la carte attendue disparaît.

![Ajout : carte découverte, nom et nombre de pièces requis](../../output/screenshots/interface-postes.png)

## 12. Administrateur : tester et renommer un poste

**Emplacement :** **Gestion des postes > Postes configurés**.

### Tester la communication

1. Repérer le poste et son IP.
2. Cliquer sur **Ping**.
3. Lire **Ping réussi** ou le message d’échec.
4. En cas d’échec, contrôler alimentation, réseau, IP et secret de communication avant de commander une session.

**Résultat attendu :** vous connaissez la réponse réseau du poste. Un Ping réussi ne vérifie pas physiquement le relais ou l’afficheur.

### Renommer un poste

1. Attendre que le poste soit inactif, temps à zéro, sans reprise en attente.
2. Cliquer sur **Modifier**.
3. Changer **Nom du poste**, en conservant le tarif souhaité.
4. Cliquer sur **Enregistrer modification** et lire le résultat.
5. Vérifier le nouveau nom dans la gestion et sur la carte d’accueil.

**Résultat attendu :** le nom est actualisé. Le champ de nom est bloqué pendant une session ou une reprise.

## 13. Administrateur : modifier le tarif d’un poste

**Prérequis :** poste déjà enregistré. **Emplacement :** **Gestion des postes**.

1. Repérer le poste, puis cliquer sur **Modifier**.
2. Dans **Nombre de pièces requis**, saisir le nouveau tarif entier positif.
3. Conserver le nom actuel si seul le tarif doit changer.
4. Cliquer sur **Enregistrer modification**, puis lire le message.
5. Revenir à **Vue des postes** et vérifier le tarif et le bouton **+N**.

**Résultat attendu :** les prochaines attributions utilisent le nouveau tarif. Le changement n’altère ni le solde, ni le temps déjà acquis. Le tarif peut être modifié pendant une session et hors ligne ; le renommage reste soumis à la disponibilité du poste.


## 14. Administrateur : supprimer un poste

**Prérequis :** aucune session active, temps à zéro et aucune reprise en attente. Un poste hors ligne peut être retiré.

1. Ouvrir **Gestion des postes** et repérer son nom et son IP.
2. Vérifier que **Supprimer** est disponible. Sinon, terminer la session ou traiter la reprise avant de supprimer.
3. Cliquer sur **Supprimer**, puis lire et confirmer le nom présenté.
4. Vérifier que le poste disparaît de la liste configurée.

**Résultat attendu :** le poste est retiré de la centrale. S’il est joignable, la centrale tente aussi d’effacer son identité. Un poste qui conserve son identité et se reconnecte peut réapparaître ; traiter son état réseau avant une nouvelle association.

## 15. Administrateur : régler la durée commune

**Prérequis :** accès à **Configuration**, stockage disponible.

1. Ouvrir **Configuration**.
2. Dans **Durée par attribution (minutes)**, saisir la durée à ajouter par clic, avec un minimum d’une minute.
3. Vérifier les autres champs sans corriger le solde si ce n’est pas nécessaire.
4. Cliquer sur **Enregistrer les modifications**.
5. Lire **Configuration enregistrée**, puis vérifier la durée sur **Vue des postes**.

**Résultat attendu :** les prochaines attributions ajoutent cette durée. Les tarifs restent individuels et se modifient dans **Gestion des postes**. Le réglage ne recalcule pas le temps déjà attribué.

![Configuration générale : durée, devise et pièces disponibles](../../output/screenshots/interface-configuration.png)

## 16. Administrateur : configurer le monnayeur en TND

**Prérequis :** connaître la valeur financière d’une impulsion selon la programmation du monnayeur.

1. Dans **Configuration**, sélectionner **Dinar tunisien (TND)** dans **Devise de la caisse**.
2. Choisir **Valeur d’une impulsion (TND)** : 0,500 ; 1 ; 2 ou 5 TND.
3. Vérifier la durée et les pièces disponibles, puis cliquer sur **Enregistrer les modifications**.
4. Si la caisse contient des opérations dans une autre devise, lire la confirmation de clôture. Annuler conserve la session existante ; confirmer ouvre une nouvelle session à zéro TND.
5. Vérifier la nouvelle devise et le montant de la session active dans **Recette**.

**Résultat attendu :** les prochaines impulsions sont enregistrées en TND. La valeur représente une impulsion, qui peut être une fraction de pièce physique. Les anciennes sessions conservent leur devise et leur montant.

## 17. Administrateur : configurer le monnayeur en DZD

**Prérequis :** monnayeur programmé pour les pièces algériennes et valeur d’une impulsion connue.

1. Dans **Configuration**, sélectionner **Dinar algérien (DZD)**.
2. Choisir une valeur par impulsion parmi **5, 10, 20, 50, 100, 200 ou 500 DA**.
3. Conserver la durée et le solde souhaités, puis cliquer sur **Enregistrer les modifications**.
4. Si la caisse contient déjà des opérations en TND, confirmer sa clôture pour ouvrir une nouvelle session à zéro DZD, ou annuler.
5. Dans **Recette**, vérifier la devise de la session active et les prochains encaissements.

**Résultat attendu :** chaque session de caisse utilise une seule devise. Aucun champ de taux de change n’est à renseigner. Les réglages anciens 0,25 ; 0,50 ; 1 ou 2 DZD restent affichés comme **ancien réglage** lorsqu’une installation les utilise déjà ; ils ne sont plus proposés pour un nouveau choix. La valeur 500 DA (50 000 centimes) est également disponible par impulsion ; elle doit correspondre à la programmation du dispositif d’encaissement.

![Configuration DZD : choix de la devise et valeur d’une impulsion](../../output/screenshots/interface-configuration-dzd.png)

## 18. Administrateur : corriger une saisie ou réinitialiser les champs

**Emplacement :** **Configuration**.

### Corriger un refus de validation

1. Cliquer sur **Enregistrer les modifications** et lire le message si la saisie est refusée.
2. Corriger le champ concerné : durée positive, valeur proposée pour la devise ou solde entier non négatif.
3. Cliquer à nouveau sur **Enregistrer les modifications**.
4. Attendre un message de réussite ; un message d’erreur ne confirme pas l’enregistrement.

**Résultat attendu :** les paramètres sont enregistrés seulement après validation. Un changement de devise sur une caisse non vide exige une clôture confirmée.

### Abandonner les modifications non enregistrées

1. Cliquer sur **Réinitialiser les champs**.
2. Vérifier le retour aux dernières valeurs chargées.
3. Choisir à nouveau un fichier de restauration si nécessaire : cette action efface aussi la sélection du fichier JSON dans le navigateur.

**Résultat attendu :** le formulaire est rétabli sans effacer les données de la centrale.


## 19. Administrateur : clôturer une session de caisse

**Prérequis :** accès à **Configuration** ; aucun encaissement en cours pendant la modification.

1. Conserver la devise actuelle, ou choisir la nouvelle devise et une valeur d’impulsion correspondante.
2. Pour clôturer en conservant la devise, cliquer sur **Clôturer la caisse et ouvrir une nouvelle session**. Ce bouton enregistre les réglages présents dans le formulaire.
3. Lire le numéro de session, son montant et sa devise dans la confirmation.
4. Confirmer pour ouvrir une nouvelle session à zéro, ou annuler pour conserver la session ouverte.
5. Dans **Recette**, vérifier la session active et sélectionner l’ancienne dans **Session de caisse** pour consulter son historique.

**Résultat attendu :** l’ancienne recette reste consultable. Les pièces disponibles et les temps de jeu restent conservés. Exemple : 5 000 DZD archivés, puis une nouvelle caisse à 0 TND après changement de devise. Clôturer ne libère pas l’espace de stockage.

## 20. Administrateur : corriger le solde partagé

**Prérequis :** connaître le solde final voulu et coordonner la correction avec les autres opérateurs.

1. Ouvrir **Configuration** et lire **Pièces disponibles**.
2. Saisir le solde total voulu, entier de 0 à 2 147 483 647. Le champ remplace le solde ; il ne représente pas un nombre de pièces à ajouter.
3. Vérifier la valeur et les autres paramètres du formulaire.
4. Cliquer sur **Enregistrer les modifications**.
5. Revenir à **Vue des postes** et vérifier le nouveau solde partagé.

**Résultat attendu :** le solde est corrigé sans écriture de recette. Cette action ne change ni les tarifs, ni le temps des sessions en cours.

## 21. Administrateur : exporter une sauvegarde

**Prérequis :** compte administrateur et navigateur autorisant les téléchargements. **Emplacement :** **Configuration > Sauvegarde et restauration**.

1. Cliquer sur **Exporter JSON**.
2. Attendre le message **La sauvegarde complète a été téléchargée**.
3. Vérifier la présence du fichier `.json` dans les téléchargements du navigateur.
4. Conserver le fichier dans un emplacement protégé et identifié pour cette centrale.

**Résultat attendu :** une sauvegarde V4 conserve paramètres, pièces disponibles, tarifs, comptes, token API, Wi-Fi/IP et préférence locale d’affichage des indicateurs.

Le fichier contient des secrets. Il ne contient pas le journal financier LittleFS, les sessions physiques en cours, les postes simplement découverts ou les logs RAM. Exporter séparément les rapports financiers utiles.

## 22. Administrateur : restaurer une sauvegarde

**Prérequis :** sauvegarde complète de la centrale, accès à un compte qu’elle contient et période de maintenance adaptée.

1. Ouvrir **Configuration > Sauvegarde et restauration**.
2. Cliquer sur **Choisir un fichier**, ou déposer la sauvegarde dans la zone prévue.
3. Sélectionner un fichier `.json` de 24 Kio maximum et vérifier le nom affiché. Le JSON doit être valide ; sa syntaxe seule ne garantit pas qu’il s’agit d’une sauvegarde complète.
4. Cliquer sur **Restaurer la sauvegarde**.
5. Lire la confirmation : réglages, comptes, postes et réseau actuels seront remplacés. Confirmer seulement si le fichier est celui voulu.
6. Attendre la réponse, puis se reconnecter avec un compte de la sauvegarde lorsque demandé.
7. Vérifier réglages, tarifs et comptes. Les paramètres réseau restaurés s’appliquent au prochain redémarrage de la centrale.

**Résultat attendu :** la configuration persistante est remplacée et les sessions web sont fermées. Le journal financier LittleFS et les sessions physiques des postes ne sont pas restaurés par ce JSON. En cas d’erreur de stockage, relever le message et suivre le diagnostic avant toute nouvelle opération.


## 23. Administrateur : consulter la recette et l’historique

**Prérequis :** accès au module **Recette**.

1. Lire le bandeau d’état : disponibilité de LittleFS, occupation et éventuelle attente de synchronisation NTP.
2. Dans **Session de caisse**, conserver **Session active** ou choisir une session archivée. Une ancienne installation peut présenter **Archive antérieure** séparément pour TND et DZD.
3. Lire **Recette de la période** et **Total de la session**, dans la devise indiquée.
4. Consulter **Impulsions encaissées**, **Crédits historiques / pièces** et **Écritures persistées**.
5. Lire le graphique et le tableau **Détail**. Les corrections manuelles du solde ne constituent pas des encaissements.
6. Si des écritures sont non datées, vérifier l’horloge. Elles sont comprises dans le total de session, mais restent hors des périodes calendaires.

**Résultat attendu :** vous distinguez les crédits disponibles de l’argent encaissé. Les montants DZD et TND ne sont jamais additionnés dans une même recette.

![Recette : session, période, exports et synthèse dans la devise de la caisse](../../output/screenshots/interface-recette.png)

## 24. Administrateur : choisir une période

1. Choisir d’abord la **Session de caisse** à consulter.
2. Dans **Regrouper par**, sélectionner **Jour**, **Semaine** ou **Mois**.
3. Dans **Date de référence**, choisir une date de la période souhaitée.
4. Attendre le chargement et vérifier la plage affichée au-dessus du graphique et du tableau.
5. Utiliser **Actualiser**, à droite du bandeau de stockage, pour relire les données.

**Résultat attendu :** le mode Jour détaille les heures ; Semaine et Mois détaillent les jours. Le total de période dépend des dates choisies, tandis que le total de session couvre toute la session.

## 25. Administrateur : exporter en PDF ou CSV

**Prérequis :** session et période chargées, journal de recette disponible.

1. Vérifier **Session de caisse**, **Regrouper par** et **Date de référence**.
2. Sous **Exporter la recette**, cliquer sur **PDF** ou **CSV**.
3. Enregistrer le téléchargement, puis ouvrir le fichier.
4. Vérifier le numéro de session, sa devise, les dates, la synthèse et les détails avant impression ou archivage.

**PDF :** en-tête PS TIME MANAGER, synthèse, tableau de montants et compteurs, pagination et rappel des colonnes sur les pages suivantes.

**CSV :** texte UTF-8 avec marqueur BOM et séparateur point-virgule ; synthèse suivie du détail, devise explicite et ligne de total. Si nécessaire, choisir UTF-8 et le séparateur point-virgule lors de l’import dans le tableur.

**Résultat attendu :** l’export correspond à la session et à la période choisies. Les écritures non datées sont signalées séparément. Un rapport PDF/CSV n’est pas une sauvegarde permettant de restaurer le journal binaire.

## 26. Administrateur : exporter toutes les sessions et vider la recette

**Prérequis :** accès administrateur, journal disponible et période de maintenance sans nouvelles pièces introduites dans le monnayeur.

1. Dans **Recette**, descendre jusqu’à **Mémoire de recette**.
2. Cliquer sur **Exporter et vider la recette** et lire l’alerte.
3. Confirmer pour préparer et télécharger le PDF complet. Il comprend les sessions contenant des écritures, la session active, toutes les dates et les écritures non datées, avec un total propre à chaque devise et session.
4. Ouvrir le PDF téléchargé, vérifier toutes les pages et conserver le fichier dans un emplacement sûr. À cette étape, la recette est toujours conservée sur la centrale.
5. Seulement après vérification, cliquer sur **PDF enregistré : vider la recette**, puis confirmer l’effacement définitif.
6. Vérifier le message de réussite, le montant de recette à zéro et l’espace libéré. La session active conserve son identifiant et sa devise.

**Résultat attendu :** seul l’historique de recette est effacé. Le solde des pièces disponibles, les postes, leurs tarifs et temps, les comptes, le Wi-Fi et la configuration sont conservés. LittleFS n’est pas formaté intégralement.

Si de nouvelles opérations arrivent après l’export, si l’export est incomplet ou si l’autorisation expire après quinze minutes, l’effacement est refusé. Relancer l’export complet avant de réessayer. En cas d’échec, lire le message et vérifier le journal ; ne pas effacer toute la flash.

![Mémoire de recette : exporter l’historique avant de confirmer son effacement](../../output/screenshots/interface-memoire-recette.png)

## 27. Administrateur : créer un compte

**Prérequis :** moins de dix comptes enregistrés. **Emplacement :** **Utilisateurs > Ajouter un utilisateur**.

1. Saisir **Nom d’utilisateur**, de 3 à 32 caractères : lettres de a à z, chiffres, point, tiret ou soulignement. Le serveur normalise le nom en minuscules.
2. Renseigner **Prénom** et **Nom**, non vides et limités à 64 caractères chacun.
3. Définir **Mot de passe**, au moins six caractères. Utiliser le bouton d’affichage pour contrôler la saisie si nécessaire.
4. Choisir **Utilisateur simple** pour l’exploitation des sessions, ou **Administrateur** pour les fonctions de gestion.
5. Cliquer sur **Ajouter l’utilisateur**.
6. Lire le message de réussite et vérifier le nouveau compte dans **Comptes existants**.

**Résultat attendu :** le compte dispose du rôle choisi et peut se connecter avec ses identifiants. Un nom déjà utilisé ou des champs invalides doivent être corrigés avant création.

![Utilisateurs : création et liste des comptes](../../output/screenshots/07-utilisateurs.jpg)

## 28. Administrateur : modifier ou supprimer un compte

**Emplacement :** **Utilisateurs > Comptes existants**.

### Modifier l’identité, le rôle ou le mot de passe d’un autre compte

1. Repérer le compte et cliquer sur **Modifier**.
2. Changer **Prénom**, **Nom** ou **Rôle** selon le besoin.
3. Pour réinitialiser le mot de passe d’un autre compte, remplir **Nouveau mot de passe (facultatif)** avec au moins six caractères. Laisser vide pour le conserver.
4. Cliquer sur **Enregistrer**, lire le résultat et vérifier les valeurs affichées.
5. Informer l’opérateur concerné qu’une reconnexion peut être nécessaire après changement de rôle ou de mot de passe.

**Résultat attendu :** les informations du compte sont actualisées. Le compte courant ne peut pas changer son propre rôle ; son propre mot de passe se modifie dans **Sécurité**. Le dernier administrateur ne peut pas être rétrogradé.

### Supprimer un compte

1. Vérifier le compte visé, puis cliquer sur **Supprimer**.
2. Lire la confirmation et confirmer la suppression du bon identifiant.
3. Vérifier sa disparition de la liste.

**Résultat attendu :** le compte est supprimé et ses sessions sont invalidées. Le compte courant et le dernier administrateur sont protégés contre la suppression.

![Éditeur de compte : identité, rôle et nouveau mot de passe facultatif](../../output/screenshots/16-modifier-utilisateur.jpg)

## 29. Administrateur : changer son mot de passe

**Prérequis :** session administrateur ouverte. **Emplacement :** **Sécurité > Mot de passe administrateur**.

1. Saisir **Nouveau mot de passe**, au moins six caractères ; choisir une valeur difficile à deviner.
2. Saisir exactement la même valeur dans **Confirmer le mot de passe**.
3. Utiliser les boutons d’affichage et les indications de concordance si nécessaire. Le bouton de validation reste désactivé tant que les champs ne sont pas valides et identiques.
4. Cliquer sur **Changer le mot de passe**.
5. Confirmer la lecture du message, puis se reconnecter avec le nouveau mot de passe lorsque demandé.

**Résultat attendu :** le nouveau mot de passe remplace l’ancien. Un utilisateur simple doit demander à un administrateur la modification de son mot de passe depuis la gestion des comptes.

![Sécurité : double saisie, indicateur de force et token API](../../output/screenshots/08-securite.jpg)

## 30. Administrateur : consulter et régénérer le token API

**Prérequis :** connaître les clients utilisant l’API et pouvoir mettre à jour leur configuration. **Emplacement :** **Sécurité > Token API**.

1. Lire la valeur affichée. Si elle est masquée, **Copier le token affiché** copie cette représentation, pas le secret complet.
2. Pour remplacer le token, cliquer sur **Régénérer le token API**.
3. Lire la confirmation indiquant que les anciens clients devront être mis à jour, puis confirmer si le remplacement est voulu.
4. Copier immédiatement le nouveau token complet et le conserver dans un emplacement protégé. Il sera masqué au prochain chargement.
5. Mettre à jour les clients autorisés avec `Authorization: Bearer VOTRE_TOKEN_API`.

**Résultat attendu :** l’ancien token ne fonctionne plus et les clients configurés avec le nouveau peuvent utiliser l’API selon les droits prévus. Le secret compilé entre centrale et postes n’est pas modifié.

L’application utilise HTTP sur le LAN. Les indications **État de la sécurité** ne remplacent pas la protection effective du réseau et des accès.

## 31. Administrateur : lire et vider le journal

**Emplacement :** **Journal**.

### Consulter les événements

1. Ouvrir **Journal**.
2. Lire le niveau, le message et l’heure relative au démarrage de chaque événement.
3. Utiliser **Actualiser** pour une lecture immédiate ; la page se recharge aussi automatiquement toutes les cinq secondes.
4. Relever le message exact si une erreur doit être diagnostiquée.

**Résultat attendu :** les événements récents aident à suivre les postes, le monnayeur et le stockage. Le journal contient au plus 100 événements en RAM et est perdu au redémarrage.

### Vider le journal

1. Relever les informations utiles avant effacement.
2. Cliquer sur **Vider le journal**.
3. Lire et confirmer le vidage définitif, ou annuler pour conserver les messages.
4. Vérifier que la liste est vidée ; de nouveaux événements peuvent ensuite apparaître.

**Résultat attendu :** les logs RAM sont effacés sans supprimer la recette LittleFS.

![Journal : messages, niveaux et commandes](../../output/screenshots/09-journal.jpg)

## 32. Administrateur : réinitialiser le Wi-Fi de la centrale

**Prérequis :** connaître le réseau de destination et pouvoir rejoindre physiquement le point d’accès de configuration après redémarrage.

1. Ouvrir le menu administrateur.
2. Cliquer sur **Réinitialiser le Wi-Fi**.
3. Lire la confirmation demandant la suppression du réseau et le redémarrage. Confirmer si l’opération est voulue.
4. Attendre le redémarrage, puis rejoindre `GAMEROOM-CENTRAL-<chipId>`.
5. Ouvrir `http://192.168.4.1` et suivre la procédure de connexion réseau de la centrale.

**Résultat attendu :** les paramètres réseau de la centrale sont effacés et son portail de configuration est ouvert. Ce bouton ne réinitialise pas les réseaux des postes.

## 33. Connecter la centrale : DHCP ou IP fixe

**Accès :** personne chargée de la mise en réseau. **Prérequis :** accès au portail de configuration de la centrale et identifiants Wi-Fi disponibles.

1. Rejoindre `GAMEROOM-CENTRAL-<chipId>` puis ouvrir `http://192.168.4.1` si nécessaire.
2. Cliquer sur **Actualiser** pour relancer la recherche de réseaux.
3. Sélectionner le réseau voulu, ou remplir **SSID** manuellement, puis saisir **Mot de passe**.
4. Conserver **Automatique (DHCP)** si le routeur attribue les IP.
5. Pour une adresse fixe, choisir **Manuelle (IP fixe)**, puis renseigner **Adresse IP fixe**, **Passerelle** et **Masque de sous-réseau**. Ajouter les DNS si nécessaires.
6. Cliquer sur **Enregistrer et connecter**. En cas d’échec, corriger les paramètres.
7. Après succès, noter l’IP, attendre le redémarrage et reconnecter l’ordinateur au LAN.
8. Ouvrir la nouvelle IP de la centrale ou `http://pstm.local`.

**Résultat attendu :** la centrale rejoint le réseau choisi et son interface est accessible. Les paramètres sont enregistrés après connexion réussie.

![Centrale : sélection du réseau et DHCP](../../output/screenshots/10-wifi-centrale.jpg)

![Centrale : adresse fixe, passerelle, masque et DNS](../../output/screenshots/19-wifi-centrale-ip-fixe.jpg)

## 34. Connecter un poste : DHCP ou IP fixe

**Accès :** personne chargée de la mise en réseau. **Prérequis :** poste en mode configuration, même réseau cible que la centrale et secret commun déjà programmé.

1. Rejoindre `GAMEROOM-POSTE-<chipId>` et ouvrir `http://192.168.4.1`.
2. Sélectionner le réseau local commun à la centrale, ou saisir son SSID ; renseigner le mot de passe.
3. Choisir DHCP, ou IP fixe avec une adresse libre propre à ce poste, la passerelle et le masque. Les DNS sont facultatifs.
4. Cliquer sur **Enregistrer et connecter**, puis attendre le résultat.
5. Après succès, rejoindre à nouveau le LAN avec le navigateur.
6. Dans la centrale, vérifier le poste dans **Gestion des postes**, puis l’associer s’il n’est pas encore configuré.

**Résultat attendu :** le poste rejoint le LAN et annonce son état. Le tarif se règle dans la centrale. Le portail `/wifi` n’est accessible que pendant le mode configuration ; il n’existe pas de bouton web local de réinitialisation Wi-Fi du poste.

![Poste : formulaire réseau en DHCP](../../output/screenshots/11-wifi-poste.jpg)

![Poste : paramètres IPv4 manuels](../../output/screenshots/20-wifi-poste-ip-fixe.jpg)

![Poste sur téléphone : formulaire après défilement](../../output/screenshots/27-wifi-poste-mobile.jpg)

## 35. Consulter directement l’état d’un poste

**Accès :** personne disposant de l’IP du poste sur le LAN. **Prérequis :** poste connecté au réseau.

1. Ouvrir `http://IP_DU_POSTE/` dans un navigateur.
2. En mode normal, vérifier la réponse **Poste OK**. Elle confirme la réponse HTTP, pas l’état physique du relais.
3. Pour plus de détails, ouvrir `http://IP_DU_POSTE/status`.
4. Lire `name`, `ip`, `status`, `relay`, `remaining`, `recoveryPending` et `recoveryRemaining`. L’état expose aussi l’identité et le marqueur de configuration.
5. Pour commander une session, revenir à la carte du poste dans la centrale.

**Résultat attendu :** vous obtenez un état JSON, dont la présentation dépend du navigateur. Le poste ne fournit pas de tableau de bord local de session.

![Réponse normale à la racine du poste](../../output/screenshots/21-poste-local.jpg)

![État JSON : identité, relais, temps et reprise](../../output/screenshots/22-poste-status.jpg)

## 36. Administrateur : résoudre un débit interrompu

**Prérequis :** message d’affectation interrompue et panneau de résolution dans **Configuration**. Ne pas confondre cet incident de débit central avec une reprise de session sauvegardée sur un poste.

1. Suspendre les nouvelles attributions et repérer le poste concerné par l’incident.
2. Vérifier directement son temps et son état pour déterminer si la commande a ajouté la durée. Un timeout ne prouve pas l’échec de la commande.
3. Ouvrir **Configuration** et lire le message associé à l’affectation interrompue.
4. Si le temps a été ajouté, cliquer sur **Confirmer le débit**, lire puis confirmer la décision.
5. Si le temps n’a pas été ajouté, cliquer sur **Annuler le débit**, lire puis confirmer la conservation des pièces.
6. Vérifier le message de résolution, le solde et la disponibilité des attributions avant de reprendre le service.

**Résultat attendu :** la centrale règle le débit incertain selon le résultat constaté. En cas de doute sur le poste ou de défaut de stockage persistant, solliciter la maintenance avant de répéter une commande.

## 37. Organiser le service et traiter les problèmes

**Accès :** utilisateur simple pour les sessions ; administrateur pour les réglages, exports et incidents de stockage.

1. À l’ouverture, consulter les postes, les reprises et le solde. L’administrateur vérifie aussi la recette, le stockage et l’horloge.
2. Avant chaque attribution, contrôler le poste demandé, son tarif et le solde suffisant.
3. Après une commande, lire son résultat et le nouvel état avant toute nouvelle action.
4. À la fermeture, vérifier les sessions restantes ; l’administrateur exporte les rapports voulus et sauvegarde les réglages après modification.
5. Se déconnecter à la fin du service.

| Problème | Action utilisateur | Action administrateur ou maintenance |
|---|---|---|
| Connexion refusée | Corriger la saisie ; demander une réinitialisation si nécessaire | Contrôler le compte et le rôle |
| Bouton +N indisponible | Lire le motif et vérifier le solde | Traiter reprise ou défaut de stockage |
| Poste hors ligne | Vérifier le bon poste et signaler la panne | Alimentation, IP, Ping, /status et secret commun |
| Résultat incertain après timeout | Ne pas répéter aveuglément | Vérifier le poste et résoudre le débit si le panneau apparaît |
| Pièce ignorée | Attendre Monnayeur prêt et signaler l’incident | Contrôler signal, GPIO13, rappel et anti-doublon |
| Recette non datée | Signaler l’anomalie | Vérifier NTP ; consulter les montants non datés |
| Stockage en erreur | Relever le message | Diagnostiquer sans formater ni effacer la flash |
| Portail absent | Vérifier le réseau rejoint | Ouvrir 192.168.4.1 et consulter le moniteur série |
