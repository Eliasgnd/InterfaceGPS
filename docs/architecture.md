# Architecture

## Vue globale

L’architecture est organisée en quatre couches :

- **Interface** : pages Qt Widgets + carte QML
- **Orchestration** : `MainWindow` coordonne l’état de l’application
- **Données** : `TelemetryData` centralise les mesures et notifications
- **Sources & services** : GPS, IMU, caméra, Bluetooth, Home Assistant

## Flux principaux

1. `main.cpp` initialise les composants applicatifs.
2. Les sources (GPS/IMU/services) publient les mises à jour vers `TelemetryData`.
3. Les pages UI s’abonnent aux signaux pour rafraîchir l’affichage.
4. `NavigationPage` transmet les actions utilisateur vers la carte QML.

## Principes de conception

- Couplage faible via signaux/slots Qt
- Modules spécialisés par domaine
- Adaptation au matériel réellement disponible
