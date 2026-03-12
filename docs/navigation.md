# Navigation

## Principe général

La navigation est gérée principalement par `NavigationPage`, qui combine :

- une vue cartographique QML,
- des contrôles Qt Widgets (recherche, zoom, recentrage, arrêt d’itinéraire),
- une synchronisation continue avec les données de télémétrie.

## Recherche de destination

Le flux standard est le suivant :

1. L’utilisateur saisit une destination (champ de recherche ou clavier virtuel `Clavier`).
2. La page déclenche la recherche de suggestions à partir d’un seuil minimal de caractères.
3. Après validation, la destination est transmise à la vue QML pour lancer la recherche d’itinéraire.

Le comportement précis des suggestions et de la recherche dépend de la couche cartographique configurée côté QML (et des services externes associés).

## Interaction avec GPS et télémétrie

`NavigationPage` se lie à `TelemetryData` via `bindTelemetry`.

Les mises à jour suivantes sont répercutées vers la carte :

- latitude,
- longitude,
- cap (heading),
- vitesse.

Cette synchronisation permet de maintenir l’icône véhicule et les informations de déplacement cohérentes avec les mesures reçues.

## Rôle de `NavigationPage`

`NavigationPage` agit comme contrôleur d’intégration entre UI Qt et carte QML :

- pilote les actions utilisateur (zoom/recentrage/recherche/arrêt),
- transmet les événements de recherche et de suggestions,
- applique les mises à jour de télémétrie sur les propriétés QML,
- gère l’affichage des éléments de recherche selon l’état de navigation.

## Limites et dépendances

- La qualité de navigation dépend de la disponibilité du signal GPS.
- Les fonctions de recherche/suggestions nécessitent une configuration cartographique valide (clé API et accès réseau, selon l’environnement).
