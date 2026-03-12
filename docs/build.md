# Compilation et exécution

## Prérequis

- Linux recommandé (ex. Ubuntu, Raspberry Pi OS).
- Qt 6 avec les modules déclarés dans `InterfaceGPS.pro`.
- Outils de build : `qmake6`, `make`, compilateur C++ compatible C++17.

Selon la cible, prévoir également les dépendances système associées à Qt WebEngine, Qt Bluetooth et Qt Multimedia.

## Compilation avec qmake

Depuis la racine du dépôt :

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
```


## Exécution

Après compilation :

```bash
./InterfaceGPS
```

Points de configuration utiles :

- `MAPBOX_API_KEY` pour la couche cartographique.
- URL Home Assistant configurable dans `homeassistant.cpp`.

## Tests Qt (unitaires/UI)

Pour lancer l’ensemble des suites de tests Qt en local depuis la racine du dépôt :

```bash
bash scripts/run_qt_tests_ci.sh
```

Ce script couvre les tests `tests/*/*_test.pro`, compile chaque suite avec `qmake6` puis exécute les binaires de test avec `xvfb-run` (utile pour les tests GUI).


## Dépannage simple

### `qmake6: command not found`

- Installer Qt 6 et l’outil qmake correspondant (`qmake6`).
- Vérifier le `PATH`.

### Erreurs de modules Qt manquants

- Installer les paquets Qt 6 requis (Widgets, QML/Quick, Positioning/Location, SerialPort, Multimedia, DBus, Bluetooth, WebEngine, etc.).
- Re-lancer `qmake6` après installation.

### L’application démarre mais certaines fonctions ne répondent pas

- Vérifier la disponibilité du matériel (GPS, IMU, Bluetooth, caméra).
- Vérifier les permissions et services système (accès série/I2C, service Bluetooth, pile réseau).

### Documentation API

Pour régénérer la documentation Doxygen :

```bash
bash scripts/run_doxygen.sh
```
