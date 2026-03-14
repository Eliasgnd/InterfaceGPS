# Accueil

InterfaceGPS est une application **Qt/C++** pour un usage embarqué combinant navigation, télémétrie, multimédia et caméra.

## Fonctionnalités clés

- Navigation cartographique avec intégration QML
- Télémétrie GPS/IMU via modèle partagé
- Pages dédiées média, caméra et paramètres
- Intégration réseau (Bluetooth, Home Assistant selon configuration)

## Modules

- **UI** : `MainWindow`, `NavigationPage`, `MediaPage`, `CameraPage`, `SettingsPage`
- **Données** : `TelemetryData`, `GpsTelemetrySource`, `Mpu9250Source`
- **Intégration** : services réseau et matériels selon la cible

## Aller à l’essentiel

- [Guide rapide (build & exécution)](build.md)
- [Architecture](architecture.md)
- [Matériel](hardware.md)
