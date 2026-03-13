# Matériel

## Plateforme cible

- Linux embarqué (Raspberry Pi ou équivalent)
- Écran HDMI/DSI (tactile recommandé)
- GPS UART/USB (NMEA)
- Caméra avec flux JPEG/UDP
- Bluetooth (intégré ou USB)
- IMU MPU9250 sur I2C (optionnel)

## Câblage de référence (Raspberry Pi)

### GPS (UART)

- `TX GPS -> GPIO15/RXD` (pin 10)
- `RX GPS -> GPIO14/TXD` (pin 8)
- `VCC/GND` selon la fiche module

### MPU9250 (I2C)

- `SDA -> GPIO2/SDA1` (pin 3)
- `SCL -> GPIO3/SCL1` (pin 5)
- `VCC -> 3.3V`, `GND -> GND`
- Adresse attendue : `0x68` sur `/dev/i2c-1`

## Préparation système

- Activer UART et I2C
- Vérifier l’accès à `/dev/i2c-1` et aux ports `/dev/tty*`
- Contrôler les permissions et services Bluetooth/réseau
