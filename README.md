# InterfaceGPS

InterfaceGPS est une interface embarquée (type tableau de bord véhicule) développée avec **Qt Widgets + QML**.

L'application regroupe en un seul écran :
- une **navigation GPS** avec carte interactive,
- une **vue caméra** temps réel (flux UDP JPEG),
- un **lecteur média Bluetooth** (MPRIS/DBus),
- une vue **Home Assistant** intégrée (WebEngine),
- des **réglages Bluetooth** (visibilité, appairage, gestion appareils),
- un mode **split-screen** (navigation + média par défaut).

---

## 1) Fonctionnalités principales

- **Navigation**
  - carte QML intégrée,
  - suivi de position véhicule,
  - recherche et autocomplétion d'adresse,
  - calcul d'itinéraire avec trafic,
  - guidage visuel (distance, durée, instruction).

- **Caméra embarquée**
  - écoute d'un flux UDP sur le **port 4444**,
  - affichage de trames JPEG en temps réel,
  - arrêt automatique du flux si la page n'est pas visible.

- **Multimédia Bluetooth**
  - contrôle lecture/pause/piste,
  - récupération des métadonnées (titre, artiste, durée),
  - intégration DBus/MPRIS pour piloter un téléphone connecté.

- **Domotique / Home Assistant**
  - affichage d'un dashboard Home Assistant dans un composant web,
  - persistance de session/cookies,
  - clavier virtuel intégré pour les champs de saisie.

- **Télémétrie véhicule**
  - GPS via port série (NMEA),
  - IMU MPU9250 via I2C (Linux),
  - données centralisées dans un modèle partagé (`TelemetryData`).

---

## 2) Prérequis système

Ce projet est prévu principalement pour Linux (ex: Raspberry Pi OS / Ubuntu) avec Qt 6.

### Matériel (optionnel mais recommandé)

- module GPS (port série, ex: `/dev/serial0`),
- capteur MPU9250 (I2C, ex: `/dev/i2c-1`),
- source caméra UDP qui envoie des JPEG sur `:4444`,
- Bluetooth actif + BlueZ,
- instance Home Assistant accessible sur le réseau local.

### Logiciels

- Ubuntu/Debian (ou dérivé),
- `qmake6` (ou `qt-cmake`), `make`, `g++`,
- modules Qt nécessaires (installés via script ci-dessous).

---

## 3) Installation automatique des dépendances Qt

Un script est fourni pour installer toutes les bibliothèques Qt et dépendances système nécessaires.

```bash
chmod +x scripts/install_qt_deps_ubuntu.sh
./scripts/install_qt_deps_ubuntu.sh
```

Le script installe notamment (version Qt6) :
- Qt Core/Gui/Widgets,
- Qt Quick/QML/QuickControls2,
- Qt Location/Positioning,
- Qt Multimedia,
- Qt SerialPort,
- Qt Bluetooth/Connectivity,
- Qt WebEngine,
- Qt Virtual Keyboard,
- outils de build et runtimes QML,
- utilitaires système (BlueZ, DBus, I2C).

---

## 4) Configuration avant lancement

### 4.1 Clé API Mapbox (navigation)

L'application lit la clé depuis la variable d'environnement `MAPBOX_API_KEY`.

```bash
export MAPBOX_API_KEY="votre_cle_mapbox"
```

Ajoutez cette ligne dans `~/.bashrc` pour la conserver.

### 4.2 URL Home Assistant

Par défaut, l'URL Home Assistant est codée en dur dans `homeassistant.cpp` :

```cpp
m_view->setUrl(QUrl("http://192.168.1.158:8123"));
```

Adaptez cette URL à votre réseau local.

### 4.3 Permissions Linux utiles

Pour accéder au série/I2C/Bluetooth, assurez-vous que l'utilisateur est dans les bons groupes :

```bash
sudo usermod -aG dialout,i2c,bluetooth $USER
```

Puis reconnectez votre session.

---

## 5) Compilation

Depuis la racine du projet :

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
```

Le binaire est généré dans le dossier courant (selon votre kit Qt / mkspec).

Si `qmake6` n'est pas disponible sur votre distribution, vous pouvez utiliser `qt-cmake` avec CMake (si vous migrez le projet vers CMake).

---

## 6) Exécution

```bash
./InterfaceGPS
```

Notes d'exécution :
- sous Linux, l'application force `QT_QPA_PLATFORM=xcb`,
- un cache cartographique est créé dans `./qtlocation_cache`,
- WebEngine est configuré pour tolérer des environnements locaux non sécurisés (utile en embarqué local).

---

## 7) Utilisation rapide (par écran)

- **Navigation** : recherchez une destination dans la barre, sélectionnez une suggestion, suivez l'itinéraire.
- **Caméra** : ouvrez l'onglet caméra pour démarrer l'écoute UDP.
- **Média** : contrôlez la lecture Bluetooth (smartphone connecté requis).
- **Settings** : gérez la visibilité Bluetooth, appareils connus et appairage.
- **Home Assistant** : ouvrez votre dashboard domotique embarqué.
- **Split** : bouton en bas pour basculer le mode écran partagé.

---

## 8) Lancer les tests

Des tests unitaires/UI existent dans `tests/`.

Exemple pour un module de test :

```bash
cd tests/telemetrydata
qmake6 telemetrydata_test.pro
make -j"$(nproc)"
./telemetrydata_test
```

Répétez la même procédure pour les autres dossiers `tests/ui_*`.

---

## 9) Dépannage

- **Carte vide / pas d'itinéraire**
  - vérifier `MAPBOX_API_KEY`,
  - vérifier l'accès internet,
  - vérifier les logs réseau Qt.

- **GPS non détecté**
  - vérifier le port série (`/dev/serial0`),
  - vérifier les permissions `dialout`,
  - vérifier le câblage/alimentation du module GPS.

- **IMU inactive**
  - vérifier `/dev/i2c-1`,
  - vérifier permissions groupe `i2c`,
  - vérifier adresse I2C capteur (`0x68`).

- **Caméra noire**
  - vérifier qu'une source envoie bien des JPEG UDP sur le port `4444`,
  - vérifier qu'aucun autre process n'occupe ce port.

- **Bluetooth/Média ne répond pas**
  - vérifier service Bluetooth actif,
  - vérifier appareil effectivement connecté en A2DP/AVRCP,
  - vérifier présence d'un service MPRIS côté système.

- **Home Assistant inaccessible**
  - vérifier IP/port de l'instance,
  - tester l'URL dans un navigateur externe,
  - ajuster l'URL dans `homeassistant.cpp`.

---

## 10) Structure du projet

- `main.cpp` : bootstrap (environnement + initialisation sources télémétrie + fenêtre principale)
- `mainwindow.*` : orchestration des pages + split-screen
- `navigationpage.*` + `map.qml` : navigation/cartographie
- `camerapage.*` : réception vidéo UDP
- `mediapage.*` + `bluetoothmanager.*` + `MediaPlayer.qml` : média Bluetooth
- `settingspage.*` : paramètres Bluetooth
- `homeassistant.*` : intégration WebEngine Home Assistant
- `telemetrydata.*` : modèle partagé de données véhicule
- `gpstelemetrysource.*` : source GPS série
- `mpu9250source.*` : source IMU I2C

---

## 11) Roadmap recommandée (optionnel)

Pour améliorer encore l'expérience utilisateur :
- rendre l'URL Home Assistant configurable via `QSettings`,
- stocker les ports GPS/caméra dans les paramètres,
- ajouter un script `run.sh` qui exporte automatiquement les variables d'environnement,
- ajouter un package `.deb` pour installation en une commande.
