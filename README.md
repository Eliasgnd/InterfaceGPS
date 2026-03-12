# InterfaceGPS

[![Documentation Doxygen](https://img.shields.io/badge/docs-doxygen-blue)](https://Eliasgnd.github.io/InterfaceGPS/)

InterfaceGPS est une application **Qt 6 / C++ / qmake** de type tableau de bord embarqué. Le projet centralise la navigation, le multimédia Bluetooth, l’affichage caméra, la télémétrie et une intégration Home Assistant dans une interface unique, pensée pour un usage véhicule/embarqué.

## Contexte et objectif

Ce dépôt vise à démontrer une architecture logicielle embarquée moderne avec Qt, utilisable à la fois comme projet pédagogique, vitrine portfolio et base technique évolutive.

Objectifs principaux :
- proposer une IHM unifiée et réactive,
- connecter des sources matérielles (GPS série, IMU I2C),
- conserver une base de code maintenable et documentée.

## Fonctionnalités

- **Navigation GPS** : carte interactive QML, recherche de destination, suggestions, itinéraires et guidage visuel.
- **Caméra embarquée** : affichage d’un flux JPEG UDP en temps réel.
- **Multimédia Bluetooth** : contrôle média via DBus/MPRIS (lecture, piste, métadonnées).
- **Home Assistant** : vue WebEngine intégrée pour la domotique embarquée.
- **Paramètres système** : page de configuration Bluetooth et états de connectivité.
- **Télémétrie unifiée** : modèle central (`TelemetryData`) partagé entre modules.
- **Mode split-screen** : affichage simultané de modules (ex. Navigation + Média).

## Stack technique

- **Langage** : C++17
- **Framework UI** : Qt 6 Widgets + QML
- **Build system** : qmake
- **Modules Qt utilisés** : `widgets`, `quickwidgets`, `qml`, `positioning`, `location`, `serialport`, `multimedia`, `dbus`, `bluetooth`, `webenginewidgets`, etc.
- **Documentation API** : Doxygen (HTML)
- **CI/CD documentation** : GitHub Actions + GitHub Pages

## Structure du projet

```text
.
├── .github/                  # Workflows CI, templates de PR/issues
├── docs/                     # Documentation projet (non API)
├── tests/                    # Tests unitaires / UI par module
├── scripts/                  # Scripts utilitaires (installation dépendances)
├── *.h / *.cpp               # Code source C++ Qt
├── *.ui / *.qml              # Interfaces Qt Designer / QML
├── InterfaceGPS.pro          # Configuration qmake
├── Doxyfile                  # Configuration Doxygen
└── README.md
```

## Compilation (qmake)

### Prérequis

- Qt 6 avec les modules nécessaires
- `qmake6`, `make`, `g++`
- Linux recommandé (Raspberry Pi OS / Ubuntu)

### Étapes

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
```

## Exécution

```bash
./InterfaceGPS
```

Variables et points de configuration utiles :
- `MAPBOX_API_KEY` pour la cartographie,
- URL Home Assistant configurable dans `homeassistant.cpp`.

## Tests

Exemple (module télémétrie) :

```bash
cd tests/telemetrydata
qmake6 telemetrydata_test.pro
make -j"$(nproc)"
./telemetrydata_test
```

## Architecture logicielle (texte)

- `MainWindow` orchestre les pages applicatives et le mode split-screen.
- `TelemetryData` joue le rôle de **source de vérité** pour les données runtime.
- `GpsTelemetrySource` et `Mpu9250Source` alimentent `TelemetryData`.
- Les pages UI (`NavigationPage`, `MediaPage`, `CameraPage`, `SettingsPage`, `HomeAssistant`) consomment/produisent des événements via Qt (signals/slots).

## Documentation

- **Guide dépôt** : `docs/README.md`
- **Documentation API générée** (Doxygen) :
  - locale : `doc_output/html/index.html`
  - publiée : https://Eliasgnd.github.io/InterfaceGPS/

## Documentation générée

Génération locale :

```bash
doxygen Doxyfile
```

Le workflow GitHub Actions publie automatiquement la documentation sur GitHub Pages pour la branche `main`.

## État du projet / roadmap

Projet actif, base fonctionnelle déjà en place.

Perspectives court terme :
- robustesse I/O matériel (gestion fine des erreurs série/I2C),
- amélioration UX tactile,
- extension des tests d’intégration multi-modules,
- packaging et déploiement embarqué reproductible.

## Captures d’écran

Aperçus UI à centraliser dans : `docs/assets/screenshots/`.

> Emplacements prévus :
> - `docs/assets/screenshots/navigation.png`
> - `docs/assets/screenshots/split_screen.png`
> - `docs/assets/screenshots/settings.png`

## Contribution

Les contributions sont bienvenues. Merci de consulter :
- `CONTRIBUTING.md`
- `CODE_OF_CONDUCT.md`
- `SECURITY.md`

## Licence

Ce projet est distribué sous licence **MIT**. Voir `LICENSE`.

## Auteur

Projet maintenu par **Eliasgnd**.
