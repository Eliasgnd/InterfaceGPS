# Matériel cible + câblage

L'application peut tourner partiellement même si tout le matériel n'est pas branché.

## 1. Matériel recommandé

- **Calculateur**: Raspberry Pi (ou autre Linux)
- **Écran**: HDMI/DSI (tactile recommandé)
- **GPS**: module UART ou USB (trames NMEA)
- **Bluetooth**: adaptateur intégré ou USB
- **Caméra**: source flux JPEG/UDP
- **IMU (optionnel)**: MPU9250 sur bus I2C

## 2. Câblage recommandé (Raspberry Pi)

### GPS (UART)

- GPS `TX` -> Raspberry Pi `GPIO15 / RXD` (pin 10)
- GPS `RX` -> Raspberry Pi `GPIO14 / TXD` (pin 8)
- GPS `VCC` -> `5V` (ou `3.3V` selon la fiche de votre module)
- GPS `GND` -> `GND`

### IMU MPU9250 (I2C)

- MPU9250 `SDA` -> Raspberry Pi `GPIO2 / SDA1` (pin 3)
- MPU9250 `SCL` -> Raspberry Pi `GPIO3 / SCL1` (pin 5)
- MPU9250 `VCC` -> `3.3V`
- MPU9250 `GND` -> `GND`
- Adresse attendue dans le code: `0x68` sur `/dev/i2c-1`

### Caméra

- Fournir un flux **JPEG/UDP** vers la machine qui exécute InterfaceGPS.

## 3. Pré-configuration OS (Raspberry Pi)

- Activer **I2C** et **UART** (ex: via `raspi-config`).
- Vérifier que le périphérique I2C est présent: `/dev/i2c-1`.
- Vérifier l'accès au port série GPS (`/dev/tty*`) selon votre branchement.

## 4. Ordre conseillé de mise en service

1. Démarrer l'application avec écran seul.
2. Ajouter le GPS et vérifier la position.
3. Ajouter Bluetooth (appairage + contrôle média).
4. Ajouter la caméra réseau.
5. Ajouter l'IMU (si utilisée).

## 5. Points d'attention système

- Permissions d'accès aux périphériques (`/dev/tty*`, I2C).
- Services actifs (Bluetooth, réseau).
- Qualité alimentation électrique sur plateforme embarquée.
- Compatibilité de niveau logique (3.3V/5V).

## 6. Compatibilité

- La cible exacte peut varier (prototype, banc, véhicule).
- Certaines fonctionnalités peuvent être indisponibles selon pilotes, kernel ou matériel.
