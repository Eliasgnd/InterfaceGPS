# Build & exécution

Ce guide est volontairement court: suivez les étapes dans l'ordre.

## 1. Prérequis

- Linux (Ubuntu / Raspberry Pi OS recommandé)
- Outils de base: `make`, compilateur C++17 (`g++`)

## 2. Installation automatique des dépendances (recommandé)

Depuis la racine du dépôt:

```bash
bash scripts/install_dependencies.sh
```

Le script installe les paquets nécessaires pour:
- compiler (`qmake6`, Qt6 dev),
- exécuter les modules (QML, WebEngine, Bluetooth, multimédia),
- lancer les tests Qt (`xvfb`),
- générer la documentation (`doxygen`).

## 3. Compiler

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
```

## 4. Lancer

```bash
./InterfaceGPS
```

## 5. Configurer

- Définir `MAPBOX_API_KEY` pour la carte.
- Adapter l'URL Home Assistant dans `homeassistant.cpp`.

## 6. Vérifier rapidement les modules

- Navigation: la carte se charge.
- Média Bluetooth: commandes visibles et réactives.
- Caméra: flux reçu côté `CameraPage`.
- Télémétrie: données GPS/IMU visibles si capteurs connectés.

## 7. Tests Qt

```bash
bash scripts/run_qt_tests_ci.sh
```

## 8. Dépannage

### `qmake6: command not found`
Relancer `bash scripts/install_dependencies.sh`.

### Modules Qt manquants
Relancer le script puis `qmake6 InterfaceGPS.pro`.

### Fonctions matérielles inactives
Vérifier:
- branchements (UART/I2C/réseau),
- permissions Linux,
- services système (Bluetooth, réseau).
