# Tarification par poste et sauvegardes V4

La centrale conserve des **pièces disponibles**, c'est-à-dire des unités de
crédit : **une impulsion physique validée ajoute une pièce**. Cette unité ne
prétend pas identifier une pièce physique : un monnayeur programmé pour envoyer
plusieurs impulsions par pièce physique ajoute autant d'unités. La devise,
la valeur monétaire par impulsion et le taux DZD/TND restent indépendants.

## Configurer et utiliser les postes

Dans **Gestion des postes**, remplir le nom et **Nombre de pièces requis** lors
de l'ajout d'un poste découvert. Le tarif initial des nouveaux postes est 1.
Le bouton **Modifier** d'un poste existant permet ensuite de changer son tarif.
Les valeurs autorisées sont les entiers de 1 à 2 147 483 647. Seul un
administrateur peut créer ou modifier un tarif.

Le tarif reste modifiable pendant une session et lorsque le poste est hors
ligne, sans commande réseau si son nom ne change pas. Le renommage et la
suppression restent interdits pendant une session ou une reprise en attente.
Une redécouverte ou un changement d'IP conserve le tarif de l'identité matérielle.

Sur l'accueil, chaque carte possède un seul bouton :

| Poste | Tarif | Bouton | Débit par clic | Temps ajouté |
|---|---:|---|---:|---|
| PS1 | 1 | +1 | 1 pièce | Une durée par attribution |
| PS2 | 4 | +4 | 4 pièces | Une durée par attribution |
| PS3 | 1 | +1 | 1 pièce | Une durée par attribution |

Avec 6 pièces disponibles, un clic sur PS2 laisse 2 pièces et un clic sur PS1
en laisse 1. Le bouton de PS2 est alors désactivé pour solde insuffisant.
Un changement de tarif n'altère ni le solde ni le temps déjà attribué.
La durée commune se règle dans **Configuration → Durée par attribution**.

Le serveur retrouve le tarif du poste et refuse les affectations insuffisantes,
les postes indisponibles, les reprises non résolues et les dépassements de durée.
Le navigateur empêche les doubles clics pendant une affectation. Une réponse
réseau d'échec ne débite pas le solde. La simulation **+1 pièce** n'ajoute aucune
recette et ne dépend d'aucun tarif de poste.

## API

Les routes conservent leurs règles d'authentification existantes.

- `GET /posts` : `availablePieces`, `creditUnit: "pulse"`, `assignmentsReady` et
  `posts[].piecesRequired`. Les anciennes propriétés de conversion globale et
  de reliquat ne sont plus exposées.
- `POST /poste/configure` (admin) : `{"chipId":"...","name":"PS2","piecesRequired":4}`.
  Si `piecesRequired` est absent à la création, il vaut 1.
- `POST /post/update` (admin) : `{"id":"...","name":"PS2","piecesRequired":4}`.
- `POST /assign` : `{"post_id":"..."}`. Un appel ajoute une seule durée de jeu,
  quel que soit le tarif. Le coût fourni éventuellement par un client est ignoré.
  L'ancien `coins: 1` est accepté ; `coins: 2` est refusé.
- `POST /config` (admin) : durée, monnaie, taux et correction facultative de
  `availablePieces`. L'ancien réglage global `pulsesPerCoin` et l'ancien solde
  `availableCoins` sont refusés pour éviter de mélanger les unités.
- `POST /pieces/simulate` (admin) : ajoute une pièce de test, sans recette.
  `/coins/simulate` reste un alias historique de cette même opération.
- `GET /config/export` et `POST /config/import` (admin) : sauvegardes V4.

Les tarifs, soldes et durées doivent être des nombres JSON entiers. Les chaînes,
booléens, fractions, valeurs négatives et dépassements sont refusés.

## Persistance et migration

La NVS conserve les instantanés vérifiés par relecture et CRC :

| Clé | Contenu |
|---|---|
| `appcfg/configV4` | Durée, devise, valeur d'impulsion et taux |
| `appcfg/creditsV4` | Solde en pièces, séquence financière locale et éventuelle séquence historique à finaliser |
| `appcfg/postsV4` | Identités et tarifs individuels des postes |
| `appcfg/restoreV4` | Restauration complète préparée, jusqu'à sa finalisation |
| `appcfg/assignmentV4` | Poste, soldes avant/après et séquence d'une affectation en cours |

En l'absence de `creditsV4`, les anciennes clés et les instantanés V3 sont lus.
Les postes existants reprennent le **dernier tarif global enregistré** et :

```text
pièces disponibles = anciens coins disponibles × dernier tarif global + reliquat
```

Le reliquat est déjà un nombre d'impulsions : il est ajouté directement, même
si son diviseur diffère du dernier tarif global. Son ancien diviseur sert à
valider qu'il est compris entre zéro inclus et le diviseur exclu. Les anciens
coins ne contiennent pas leur tarif d'acquisition historique ; le dernier tarif
global constitue donc la règle explicite de valorisation de ces coins.

Les clés historiques restent intactes. `creditsV4` est écrit en dernier et sert
de marqueur d'engagement : une migration interrompue avant ce marqueur repart
des anciennes données ; les démarrages suivants ne reconvertissent jamais le
solde déjà migré. Une donnée invalide, un CRC incorrect ou un dépassement bloque
les opérations sans remise à zéro silencieuse.

Une recette préparée avant la migration conserve son enregistrement REV2
historique, y compris son ancien nombre de coins. Après finalisation dans
LittleFS, seules ses impulsions encore non appliquées sont ajoutées au nouveau
solde, une seule fois. Les recettes nouvelles utilisent une unité par impulsion.
Les rapports désignent donc la colonne mixte **Crédits historiques / pièces**.
Aucune recette n'est créée par migration, affectation, simulation ou correction.

## Sauvegardes et restauration interrompue

Le format est `pstm-backup`, version 4, avec `creditUnit: "pulse"`,
`availablePieces` et `posts[].piecesRequired`. Il comprend aussi les comptes,
le token, la monnaie, le taux, le Wi-Fi/IP et la préférence d'affichage ajoutée
par l'interface. Le journal de recette LittleFS ne fait pas partie de cet export.
La séquence financière reste locale : elle ne vient jamais du fichier importé.

Les sauvegardes V1, V2 et V3 sont acceptées avec la même règle de migration que
la NVS. Les sauvegardes V1 n'ont aucun reliquat ; V1/V2 sont en TND. Une sauvegarde
V4 incomplète ou avec un tarif invalide est rejetée avant toute écriture.

Après validation complète, un journal de restauration canonique est préparé en
NVS. Les écritures des paramètres, postes, crédits, comptes et Wi-Fi sont
vérifiées avant suppression de ce journal. Une erreur après préparation bloque
les nouvelles opérations ; au redémarrage, la restauration entière est rejouée
avant le chargement normal. Une erreur avant préparation ne modifie aucun des
paramètres à restaurer. Une préparation dont la relecture est incertaine impose
également un redémarrage pour déterminer si elle a été engagée.

La taille JSON maximale est 24 Kio, celle des postes persistés 8 Kio et celle des
comptes sérialisés 3 900 octets. La place NVS réellement disponible peut limiter
la sauvegarde intermédiaire : les erreurs de stockage sont signalées et le
journal préparé est conservé jusqu'à reprise réussie.

## Affectation interrompue

Le débit est préparé en NVS avant l'envoi de la commande. Après succès HTTP, le
nouveau solde est sauvegardé puis le marqueur est effacé. Si le solde débité est
déjà durable après une coupure, le démarrage finalise seulement le marqueur.

Si la coupure ou l'échec NVS laisse le résultat incertain, les nouvelles
affectations et les encaissements sont suspendus. Un administrateur doit
**vérifier le poste**, puis utiliser **Configuration → Confirmer le débit** si
le temps a été ajouté, ou **Annuler le débit** sinon. La route correspondante
est `POST /assignment/resolve`, avec `{"debit":true}` ou `{"debit":false}`.
Aucune résolution automatique ne suppose que le poste a reçu une commande.

Le protocole HTTP existant ne fournit pas de garantie de livraison exactement
une fois : un timeout peut suivre une commande déjà exécutée. Il faut vérifier
le poste avant de répéter une demande ambiguë. Une impulsion restée uniquement
en RAM au moment d'une coupure ne bénéficie pas encore de la préparation NVS.

## Vérifications reproductibles

```sh
python3 tests/host/run.py --arduinojson /chemin/ArduinoJson/src
node tests/ui/pricing.mjs
```

Les tests hôtes compilent les vrais services avec une NVS et un LittleFS en
mémoire. Ils couvrent tarifs 1/4/1, démarrage/prolongation, réseau indisponible,
modification en cours de session, persistance, redécouverte, migrations V1–V3,
aller-retour V4, imports invalides, coupures aux frontières NVS, résolution d'un
débit interrompu et recette TND/DZD historique. Ils ne nécessitent aucun ESP32.

Les tests navigateur nécessitent Playwright et un Chromium compatible. Les
variables `PLAYWRIGHT_MODULE` et `CHROMIUM_EXECUTABLE_PATH` permettent d'utiliser
une installation existante. Ils chargent le véritable HTML embarqué avec une
API de démonstration pour vérifier les boutons, formulaires, droits et le mobile.
Les essais électriques et les coupures sur matériel réel restent à effectuer.
