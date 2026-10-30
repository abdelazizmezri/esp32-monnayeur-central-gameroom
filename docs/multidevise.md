# Caisse à devise unique : TND ou DZD

Une session de caisse utilise une seule devise. Tous les nouveaux encaissements,
les cartes de recette, le graphique et les exports CSV/PDF utilisent cette devise. Les crédits et les durées de jeu restent
indépendants de la devise.

## Configuration et clôture

Dans **Configuration**, choisir **Devise de la caisse**, puis la valeur d'une
impulsion, conformément à la programmation physique du monnayeur :

| Devise | Valeurs par impulsion | Unité enregistrée |
|---|---|---|
| TND | 0,500 ; 1 ; 2 ; 5 | millime (1/1000 TND) |
| DZD | 5 ; 10 ; 20 ; 50 ; 100 ; 200 | centime (1/100 DZD) |

La sélection DZD est adaptée à la salle de jeux et utilise les valeurs 5, 10,
20, 50, 100 et 200 DA. Les règlements de la [Banque d’Algérie](https://www.bank-of-algeria.dz/reglement-piece-de-monnaie/)
documentent ces pièces. Il s’agit d’un choix d’exploitation pour le monnayeur,
pas d’une déclaration de retrait légal des petites pièces. Les anciens réglages
0,25 / 0,50 / 1 / 2 DZD restent lisibles et affichés comme « ancien réglage » si
la centrale en utilise déjà un ; ils ne sont plus proposés pour un nouveau choix.
Les recettes historiques et leurs montants natifs restent inchangés.

- Sans opération, changer de devise conserve la session vide.
- Avec des opérations, **Enregistrer les modifications** demande de confirmer la
  clôture, avec le montant et la devise de l'ancienne session. Annuler conserve la
  caisse existante. Confirmer ouvre une nouvelle session à zéro dans la devise choisie.
- Le bouton **Clôturer la caisse et ouvrir une nouvelle session** permet aussi de
  clôturer en conservant la même devise. Il enregistre les réglages du formulaire.
- Une clôture conserve les crédits disponibles et toutes les anciennes opérations.

Exemple : une session de 5 000 DZD reste archivée à 5 000 DZD après un passage à
TND ; la nouvelle session commence à 0,000 TND.

La sortie électrique du monnayeur ne fournit pas l'identité de la devise. Le
logiciel utilise le réglage choisi ; adapter aussi la programmation matérielle
si nécessaire. Les valeurs représentent une impulsion, qui peut être une fraction
d'une pièce physique. L'anti-doublon matériel existant reste inchangé.

## Recette, historique et exports

La page **Recette** sélectionne par défaut la session active. Le menu **Session
de caisse** donne accès aux anciennes sessions avec leur devise et leur montant.
Choisir ensuite Jour, Semaine ou Mois et une date de référence.

**Recette de la période** concerne les dates sélectionnées. **Total de la session**
comprend toutes ses écritures, y compris celles reçues avant synchronisation NTP.
Ces dernières sont signalées séparément et exclues du graphique calendaire. Les corrections manuelles du solde restent exclues de la recette.

CSV et PDF exportent la session et la devise sélectionnées. Aucun total ne mélange
DZD et TND. L'API `/revenue/data` et `/revenue/export.csv` accepte `period`, `anchor`,
`session` et `currency` ; sans session ni devise, elle utilise la caisse active.
Les montants courants sont `totalMinor`, `sessionAmountMinor`, `undatedMinor` et
`items[].amountMinor`, avec `currency` pour leur unité.

## Persistance et migration

La configuration NVS `configV4` conserve l'identifiant de session dans ses octets
24–27, anciennement réservés, et abandonne l'ancien taux. Les nouvelles écritures
REV2 conservent l'identifiant aux octets 64–67, couverts par le CRC. Les champs
historiques de taux et de conversion restent lisibles pour conserver les anciens
fichiers, mais aucune nouvelle conversion n'est effectuée et ces champs ne servent
plus aux rapports. La compaction ne fusionne jamais deux sessions différentes.

Au premier démarrage de cette version, après récupération d'une éventuelle
transaction préparée, la centrale ouvre la session 1 à zéro. Les anciennes
écritures REV1/REV2 restent dans **Archive antérieure**, session 0. Si elles
contiennent déjà les deux devises, le menu présente deux archives distinctes,
**TND** et **DZD**, calculées uniquement à partir des montants natifs conservés.
Aucun historique n'est effacé ni réétiqueté.

Lors d'un changement de configuration, la capture des impulsions est suspendue,
la file existante est sauvegardée, puis la nouvelle devise et la nouvelle session
sont écrites dans le même instantané NVS. Faire le changement lorsque le monnayeur
est au repos. Les écritures déjà préparées restent récupérables après coupure.

L'API POST `/config` refuse un changement de devise sur une caisse non vide
avec HTTP 409. Pour confirmer la clôture, envoyer `closeRevenueSession: true`
et `expectedRevenueSessionId` égal à l'identifiant courant. Une confirmation
ancienne est refusée. Les taux reçus par cette API sont refusés.

Les sauvegardes JSON V1 à V4 restent importables ; leurs anciens taux sont ignorés.
Les nouveaux exports V4 n'incluent aucun taux. L'historique LittleFS n'est pas
contenu dans ces sauvegardes : une restauration conserve l'identifiant local de
session et ne peut changer la devise d'une caisse non vide. Clôturer d'abord.

## Export complet et vidage de l’historique

En bas de **Recette**, le bloc **Mémoire de recette** propose **Exporter et vider
la recette**. Après l’alerte initiale, un PDF réunit les sessions contenant des
écritures et la session active sur toutes les dates, y compris les écritures non
datées. Chaque session conserve sa devise et son total.

Ouvrir le PDF, vérifier son contenu et l’enregistrer durablement. L’historique
reste conservé tant que l’administrateur n’a pas cliqué sur **PDF enregistré :
vider la recette**, puis confirmé l’effacement définitif. Le PDF est un rapport,
pas une sauvegarde binaire réimportable.

Le serveur prépare l’archive avec `POST /revenue/archive/prepare`, la transmet par
pages de 32 écritures avec `GET /revenue/archive/page?token=...&offset=...`, puis
accepte `POST /revenue/clear?token=...` avec `pdfSaved: true` après lecture complète.
Le jeton expire au bout de quinze minutes. La séquence, la session, la devise,
la taille et l’empreinte du journal doivent toujours correspondre à l’archive.
Une nouvelle opération ou un export incomplet bloque donc l’effacement.

Le remplacement concerne uniquement `/revenue.bin`, sans formatage de LittleFS.
Un en-tête de 16 octets et un repère de reprise de 72 octets conservent la cohérence
des séquences NVS. Le solde, la configuration, les comptes, le réseau, les postes
et leurs temps ne sont pas effacés. La session active garde son identifiant et sa
devise, et sa recette repart à zéro. Effectuer l’opération au repos du monnayeur.

## Capacité et exploitation

La partition LittleFS fait 640 Kio. À 72 octets par écriture REV2, le plafond brut
est proche de 9 000 écritures avant frais du système de fichiers. Un lot peut
représenter plusieurs impulsions. La compaction horaire regroupe les écritures
compatibles d’une même session, sans confondre deux sessions.

La capacité utile dépend donc de l’activité et des changements de session ou de
valeur ; la compaction exige aussi de l’espace temporaire. Surveiller l’occupation
sur la page Recette, exporter régulièrement et libérer l’historique au besoin.
Aucune ancienne recette n’est supprimée automatiquement pour gagner de la place.
