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

## Documentation Doxygen (GitHub Pages)

La documentation API générée par Doxygen est publiée via la branche `gh-pages`.

- **Lien principal (index Doxygen)** : `https://<votre-utilisateur>.github.io/InterfaceGPS/`
- **Lien HTML explicite** (si votre génération est dans un sous-dossier) : `https://<votre-utilisateur>.github.io/InterfaceGPS/doxygen/html/index.html`

> Remplacez `<votre-utilisateur>` par votre nom d'utilisateur GitHub.
>
> Exemple : `https://toto.github.io/InterfaceGPS/`

---

## 1) Démarrage rapide (objectif : « je branche une Pi 4 et ça marche »)

### Étape 1 — Matériel recommandé

- Raspberry Pi 4 (Raspberry Pi OS Bookworm ou Ubuntu 22.04+),
- écran HDMI tactile ou écran classique,
- module GPS série (ex: NEO-6M/NEO-M8N),
- capteur IMU MPU9250 (I2C),
- caméra/récepteur vidéo qui envoie des JPEG en UDP sur le port `4444`,
- smartphone Bluetooth (pour la partie média),
- accès réseau local (pour Home Assistant) + internet (Mapbox).

### Étape 2 — Câblage type (Pi 4)

> Vérifiez **toujours** la tension de vos modules (3.3V/5V) avant branchement.

#### GPS (UART)

- VCC -> 5V (ou 3.3V selon module)
- GND -> GND
- TX GPS -> RX Pi (GPIO15 / pin 10)
- RX GPS -> TX Pi (GPIO14 / pin 8)

#### MPU9250 (I2C)

- VCC -> 3.3V
- GND -> GND
- SDA -> GPIO2 (pin 3)
- SCL -> GPIO3 (pin 5)

### Étape 3 — Activer les interfaces Pi

```bash
sudo raspi-config
```

Activez :
- `Interface Options` -> `I2C` -> Enable,
- `Interface Options` -> `Serial Port` ->
  - shell login over serial: **No**,
  - serial hardware: **Yes**,
- Bluetooth (normalement actif par défaut sur Pi 4).

Puis redémarrez.

### Étape 4 — Vérifier les périphériques

```bash
ls /dev/i2c-1
ls /dev/serial0
```

Et pour l'IMU (adresse attendue `0x68`) :

```bash
sudo apt-get update
sudo apt-get install -y i2c-tools
sudo i2cdetect -y 1
```

### Étape 5 — Installer les dépendances de build/runtime

```bash
chmod +x scripts/install_qt_deps_ubuntu.sh
./scripts/install_qt_deps_ubuntu.sh
```

### Étape 6 — Configurer les variables essentielles

#### Clé Mapbox

```bash
export MAPBOX_API_KEY="votre_cle_mapbox"
```

Ajoutez-la dans `~/.bashrc` pour la rendre persistante.

#### URL Home Assistant

Par défaut, l'URL est codée ici :

```cpp
m_view->setUrl(QUrl("http://192.168.1.158:8123"));
```

Adaptez la valeur dans `homeassistant.cpp` selon votre réseau.

### Étape 7 — Donner les permissions utilisateur

```bash
sudo usermod -aG dialout,i2c,bluetooth $USER
```

Déconnectez-vous/reconnectez-vous ensuite.

### Étape 8 — Compiler

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
```

### Étape 9 — Lancer

```bash
./InterfaceGPS
```

Si tout est correctement branché/configuré, vous devez pouvoir :
- voir la carte + faire un itinéraire,
- recevoir les données GPS/IMU,
- voir la caméra dans l'onglet caméra,
- piloter le média Bluetooth,
- afficher Home Assistant.

---

## 2) Fonctionnalités principales

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

## 3) Prérequis système

Ce projet est prévu principalement pour Linux (ex: Raspberry Pi OS / Ubuntu) avec Qt 6.

### Logiciels

- Ubuntu/Debian (ou dérivé),
- `qmake6` (ou `qt-cmake`), `make`, `g++`,
- modules Qt nécessaires (installés via script ci-dessus).

---

## 4) Utilisation rapide (par écran)

- **Navigation** : recherchez une destination dans la barre, sélectionnez une suggestion, suivez l'itinéraire.
- **Caméra** : ouvrez l'onglet caméra pour démarrer l'écoute UDP.
- **Média** : contrôlez la lecture Bluetooth (smartphone connecté requis).
- **Settings** : gérez la visibilité Bluetooth, appareils connus et appairage.
- **Home Assistant** : ouvrez votre dashboard domotique embarqué.
- **Split** : bouton en bas pour basculer le mode écran partagé.

---

## 5) Lancer les tests

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

## 6) Dépannage

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

## 7) Structure du projet

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

## 8) Roadmap recommandée (optionnel)

Pour améliorer encore l'expérience utilisateur :
- rendre l'URL Home Assistant configurable via `QSettings`,
- stocker les ports GPS/caméra dans les paramètres,
- ajouter un script `run.sh` qui exporte automatiquement les variables d'environnement,
- ajouter un package `.deb` pour installation en une commande.
