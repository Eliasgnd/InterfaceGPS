# Architecture logicielle

## Vue d’ensemble

L’application repose sur une architecture Qt classique :

- une fenêtre principale (`MainWindow`) qui orchestre l’affichage,
- des pages fonctionnelles spécialisées (navigation, média, caméra, paramètres),
- un modèle de télémétrie partagé (`TelemetryData`) alimenté par des sources matérielles,
- des interactions basées sur le mécanisme **signals/slots** de Qt.

L’interface est pensée pour un contexte embarqué, avec une navigation rapide entre modules et un mode d’affichage partagé.

## Modules principaux et responsabilités

### `MainWindow`

- Point d’orchestration de l’IHM.
- Instancie les pages applicatives et gère leur affichage.
- Gère la bascule entre mode plein écran et mode split-screen.
- Connecte la télémétrie à la page de navigation.

### `NavigationPage`

- Gère la carte et les interactions de navigation.
- Prend en charge la recherche de destination et les suggestions.
- Relie les données GPS/télémétrie aux propriétés de la vue cartographique QML.
- Pilote l’affichage des contrôles de recherche pendant/hors guidage.

### `MediaPage`

- Expose la couche multimédia côté interface.
- S’appuie sur `BluetoothManager` pour piloter lecture/pause, piste suivante/précédente et métadonnées MPRIS.

### `CameraPage`

- Affiche un flux caméra reçu en UDP (frames JPEG).
- Active/arrête l’écoute réseau en fonction de la visibilité de la page pour limiter la charge.

### `SettingsPage`

- Regroupe des actions de configuration utilisateur/système.
- Couvre notamment des opérations Bluetooth (visibilité/appairage et gestion de périphériques connus selon la configuration locale).

### `TelemetryData`

- Modèle central partagé des données runtime (position, cap, vitesse, état GPS).
- Sert de point de synchronisation entre sources capteurs et pages UI.
- Émet des signaux de changement consommés par les vues.

### `GpsTelemetrySource`

- Lit un flux NMEA depuis un port série.
- Convertit les informations de position en valeurs applicatives.
- Met à jour `TelemetryData` en continu.

### `Mpu9250Source`

- Source inertielle / orientation liée au capteur MPU9250 (I2C).
- Alimente la télémétrie sur les grandeurs disponibles selon la configuration matérielle et logicielle effective.

### `BluetoothManager`

- Interface vers DBus/MPRIS pour le contrôle média Bluetooth.
- Découvre les services lecteurs, lit les métadonnées et l’état de lecture, expose des commandes de contrôle.

### `Clavier`

- Clavier virtuel intégré pour la saisie tactile.
- Utilisé notamment dans la recherche de destination pour éviter de dépendre d’un clavier système.

## Interactions principales entre modules

1. **Démarrage**
   - `main.cpp` instancie `TelemetryData`, puis les sources matérielles (`GpsTelemetrySource`, `Mpu9250Source`) et la `MainWindow`.
2. **Chaîne télémétrie**
   - Les sources capteurs écrivent dans `TelemetryData`.
   - Les pages intéressées (en particulier `NavigationPage`) reçoivent les signaux et mettent à jour l’affichage.
3. **Chaîne navigation**
   - `NavigationPage` relaie la recherche/suggestions vers la couche cartographique QML.
   - Les actions utilisateur (recherche, arrêt d’itinéraire, recentrage) déclenchent des appels vers les méthodes QML.
4. **Chaîne multimédia Bluetooth**
   - `MediaPage` utilise `BluetoothManager` pour récupérer l’état MPRIS et envoyer des commandes de transport.
5. **Chaîne caméra**
   - `MainWindow` contrôle l’activation du flux de `CameraPage` selon la page affichée.

## Notes

- Le découplage repose principalement sur les signaux/slots Qt et sur le modèle partagé `TelemetryData`.
- Les capacités exactes dépendent des périphériques réellement disponibles sur la cible.
