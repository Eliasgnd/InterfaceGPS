# Plateforme matérielle

## Objectif matériel

InterfaceGPS cible un environnement embarqué (prototype ou véhicule), avec une logique applicative Qt exécutée sur une plateforme Linux.

## Éléments matériels visés

### Raspberry Pi (ou plateforme Linux équivalente)

- Héberge l’application Qt.
- Exécute les modules UI, navigation, multimédia et intégrations système.
- Sert de point d’interface vers les périphériques (série, I2C, réseau, Bluetooth).

### Récepteur GPS

- Fournit la position, la vitesse et potentiellement le cap via trames NMEA.
- Connecté typiquement en série (UART/USB selon montage).
- Exploité par la source `GpsTelemetrySource`.

### IMU / MPU9250

- Fournit des données inertielles et/ou d’orientation selon la configuration du capteur.
- Utilisée par `Mpu9250Source` lorsque le matériel et le bus I2C sont disponibles.

### Caméra embarquée

- Produit un flux image (dans le projet actuel, réception UDP/JPEG côté application).
- Affichée via `CameraPage`.
- Peut être une caméra locale ou une source réseau selon l’architecture déployée.

### Bluetooth

- Utilisé pour l’intégration multimédia (contrôle lecteur via MPRIS/DBus) et la gestion d’appairage.
- Dépend de l’adaptateur Bluetooth disponible sur la cible.

### Écran / interface tactile

- L’IHM est conçue pour un affichage embarqué fixe.
- Un usage tactile est prévu (clavier virtuel intégré).

## Remarques de compatibilité

- Le matériel exact peut varier entre environnement de développement et cible finale.
- Certaines fonctions (capteurs, Bluetooth, caméra) peuvent être partiellement disponibles selon les pilotes, permissions système et services actifs.
