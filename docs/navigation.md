# Navigation

Cette page décrit la logique métier de navigation, centrée sur `NavigationPage`.

## Responsabilités

- Gestion des actions utilisateur (recherche, zoom, recentrage, arrêt)
- Pont entre Qt Widgets et carte QML
- Synchronisation des données GPS (latitude, longitude, cap, vitesse)

## Flux métier

1. Saisie d’une destination (champ de recherche / clavier virtuel).
2. Envoi des requêtes de suggestions et d’itinéraire vers la carte QML.
3. Mise à jour de la position véhicule via `TelemetryData`.

## Dépendances

- Clé cartographique valide (`MAPBOX_API_KEY`)
- Connectivité réseau selon le fournisseur cartographique
