# Recettes TND / DZD

Évolution du 20 octobre 2026, centrale ESP32. Les postes conservent leur firmware
et leurs règles d'attribution de temps.

## Réglages et utilisation

Dans **Configuration**, choisir la devise active puis la valeur financière d'une
impulsion, en accord avec la programmation matérielle du monnayeur :

| Devise | Valeurs sélectionnables | Unité stockée |
|---|---|---|
| TND | 0,500 ; 1 ; 2 ; 5 TND | millime (1/1000 TND) |
| DZD | 0,25 ; 0,50 ; 1 ; 2 ; 5 ; 10 ; 20 ; 50 ; 100 ; 200 DZD | centime (1/100 DZD) |

Les valeurs DZD correspondent aux séries publiées par la
[Banque d'Algérie](https://www.bank-of-algeria.dz/reglement-piece-de-monnaie/).
La sortie électrique du monnayeur ne donne pas d'identité de devise : le logiciel
utilise la devise et la valeur configurées. Il ne reconnaît pas automatiquement
une pièce tunisienne parmi des pièces algériennes. La valeur choisie représente
**une impulsion**, qui peut être une fraction de la valeur d'une pièce programmée
pour produire plusieurs impulsions. L'anti-doublon matériel existant de 320 ms
reste inchangé ; adapter sa constante seulement après mesure d'un train multiple.

Le réglage **1 DZD = X TND** se trouve dans Configuration. Il accepte une virgule
ou un point et jusqu'à six décimales, de 0,000001 à 1000 TND. Il est manuel et
ne nécessite aucun service de change en ligne. Sur une installation migrée,
aucun taux fictif n'est choisi : le TND fonctionne, mais le DZD ne peut être
activé avant saisie d'un taux positif. La page Recette affiche le taux actuel
et un lien pour le modifier.

Chaque impulsion capture sa devise, sa valeur et son taux dans une file RAM
avant traitement. Les impulsions reçues avant un changement gardent leur ancien
profil monétaire. Chaque impulsion validée ajoute une pièce disponible ; chaque
poste possède son propre nombre de pièces requis pour une durée de jeu. Le taux
de change sert uniquement à la comptabilité et ne modifie ni le tarif du poste
ni le temps attribué.

## Calcul et rapports

Pour chaque période jour/semaine/mois, les cartes, le tableau et les exports
CSV/PDF contiennent : TND encaissés, DZD encaissés, total général converti en TND.
Le graphique représente ce total général en TND ; son infobulle donne les deux
montants d'origine. Le PDF est paginé lorsque le détail dépasse une page.

Le taux est un entier en micro-TND par DZD (`dzdToTndRateMicros`, échelle 1 000 000).
La conversion d'une impulsion DZD est :

```text
millimes = (centimes × tauxMicroTND + 50 000) / 100 000, division entière
```

C'est un arrondi au millime le plus proche, demi vers le haut, **par impulsion**.
Les lots et les rapports additionnent ces conversions déjà arrondies ; la
compaction ne recalcule jamais une conversion à partir du montant agrégé.
Exemple fictif : 10 TND + 100 DZD au taux de 0,020000 donnent 12,000 TND.
Après passage du taux à 0,030000, cette recette reste à 12,000 TND. Un nouveau
versement de 100 DZD porte le total à 15,000 TND.

Les simulations et corrections manuelles du solde n'ajoutent aucune recette.
Sans heure NTP valide, un encaissement conserve ses montants et son taux mais
reste non daté : ses trois montants sont signalés séparément et exclus des
périodes calendaires. Les fichiers exportés indiquent aussi ces montants non datés.

## Persistance et reprise

La NVS `appcfg` conserve les instantanés CRC `configV4` (devise, valeur, taux,
durée), `creditsV4` (solde en pièces, dernière séquence) et `postsV4` (identités
et tarifs individuels). Chaque instantané est écrit en une opération puis relu.

Pour un encaissement :

1. Préparer en NVS `revenue/pending` une transaction contenant le profil figé,
   les montants, une séquence unique et l'état des crédits après application.
2. Ajouter son enregistrement de 72 octets au journal LittleFS REV2, fermer et
   relire les octets écrits.
3. Enregistrer atomiquement les crédits et la séquence dans `creditsV4`.
4. Effacer la préparation NVS.

Au démarrage, une préparation présente est rejouée : si sa séquence est déjà
dans LittleFS, la recette n'est pas réécrite ; si elle est déjà dans les crédits,
les crédits ne sont pas ajoutés une deuxième fois. Une relecture NVS incertaine
en fonctionnement est aussi rapprochée de la séquence avant de retenter une
impulsion conservée en RAM. Aucune écriture flash n'a lieu dans l'ISR GPIO.

Une écriture finale incomplète est réparée en conservant le préfixe valide puis
la préparation NVS complète l'opération. Un dernier bloc complet avec CRC invalide
n'est réparé que si sa préparation NVS permet sa reconstruction. Sinon, comme
pour une corruption intérieure, le logiciel bloque le
journal et conserve l'original, sans recréer un journal vide. La migration et la
compaction utilisent `revenue.tmp` et `revenue.bak` ; après interruption d'un
remplacement, un fichier complet est récupéré. Le journal principal reste
prioritaire s'il existe, même avec une fin déchirée. La compaction fusionne
seulement des entrées compatibles (heure, devise, taux et valeur d'impulsion)
et préserve les montants, le nombre d'événements et la dernière séquence.

Si le stockage est indisponible ou plein, la transaction préparée reste en NVS
et les crédits correspondants attendent sa finalisation. Les changements de
configuration, restaurations, simulations et affectations sont refusés pendant
une opération financière non finalisée. Le statut explique le problème.

## Migration et sauvegardes

Les anciennes clés NVS sont lues avec leur type d'origine, puis migrées vers les
instantanés V4, avec migration du tarif et du solde décrite dans
[Tarification par poste](tarification-par-poste.md). Les journaux REV1 sont convertis vers REV2 : tous leurs montants
sont des TND, sans conversion ni application rétroactive d'un taux DZD.
Ne pas effacer la flash ou formater LittleFS pour installer cette mise à jour.
Prévoir de la place libre pour la copie de migration ; si elle échoue, le fichier
original reste conservé. Le retour à l'ancien firmware ne lit pas les nouvelles
données : garder une sauvegarde préalable avant une mise à jour de production.

L'export de configuration est `pstm-backup` version 4 : `creditUnit: "pulse"`,
`availablePieces`, `posts[].piecesRequired`, `coinCurrency`, `pulseValueMinor` et
`dzdToTndRateMicros`. Les sauvegardes V1/V2 restent acceptées en TND ; V3 conserve
sa devise et son taux. Leur solde et leurs postes sont migrés vers les unités V4.
`pulseValueMillimes` reste un alias API en mode TND. La séquence financière est
locale et n'est jamais restaurée depuis un fichier de configuration. Le JSON
ne contient pas le journal financier. Un journal NVS permet de terminer une
restauration interrompue au redémarrage.

Exemple de modification par l'API administrateur authentifiée :

```json
{
  "coinCurrency": "DZD",
  "pulseValueMinor": 10000,
  "dzdToTndRateMicros": 20000
}
```

Cet exemple définit une impulsion de 100 DZD et un taux **fictif** de 0,020000.
`availablePieces` est facultatif ; l'interface ne l'envoie que si l'opérateur
modifie effectivement le solde, pour éviter d'écraser des crédits reçus entretemps.

`GET /revenue/data` conserve `totalMillimes` / `items[].amountMillimes` pour le
total général et ajoute `totalTndMillimes`, `totalDzdCentimes`,
`currentRateMicros`, `items[].tndMillimes`, `items[].dzdCentimes` et les équivalents
non datés. La politique de conversion est `historical-per-pulse-half-up`.

## Compilation et aperçu

Les deux firmwares ont été compilés avec Arduino CLI 1.5.1, le cœur ESP32 3.3.12
et ArduinoJson 7.4.3, avec `-Wall -Wextra`. Les écrans Configuration/Recette ont
été contrôlés dans le navigateur, y compris le refus d'un taux nul. Les PDF
exportés ont été rendus pour vérifier leurs colonnes et leur pagination.

Le serveur `docs/manuals/mock_server.py` sert les écrans embarqués avec des
montants TND/DZD fictifs et un taux de démonstration, sans accéder aux ESP32.
Les PDF illustrés précédemment publiés restent ceux du 25 août 2026.

## Limites matérielles et essais sur appareil

Une impulsion encore dans l'ISR ou la file RAM et **pas encore préparée en NVS**
ne peut pas être garantie après disparition de l'alimentation. La file contient
au maximum 127 impulsions ; sa saturation est journalisée. Une alimentation
secourue ou une réserve d'énergie permettant de finir l'écriture est nécessaire
pour couvrir physiquement cette fenêtre. L'anti-doublon et les impulsions ignorées
pendant les cinq secondes de démarrage restent ceux du projet existant.

Sur une centrale réelle, vérifier le profil électrique et chaque valeur de
pièce, puis couper et rétablir l'alimentation pendant des encaissements, avant
et après modification du taux. Contrôler l'absence de doublons, le solde, les
tarifs des postes et les trois totaux après plusieurs redémarrages. Répéter autour d'une
migration/compaction et avec le réseau/NTP indisponible.
