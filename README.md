# InterfaceGPS

[![Documentation Doxygen](https://img.shields.io/badge/docs-doxygen-blue)](https://Eliasgnd.github.io/InterfaceGPS/)

InterfaceGPS est une application **Qt 6 / C++ / qmake** pour écran embarqué (ex: Raspberry Pi), qui regroupe navigation, multimédia Bluetooth, caméra et télémétrie dans une interface unique.

## 1) Démarrage rapide (objectif: première exécution en < 15 min)

### Prérequis minimum

- Linux (Ubuntu ou Raspberry Pi OS recommandé)
- `make`, `g++`

### Installation automatique des dépendances (recommandé)

```bash
bash scripts/install_dependencies.sh
```

### Compiler

Depuis la racine du dépôt:

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
```

### Lancer

```bash
./InterfaceGPS
```

Si l'application démarre, votre environnement logiciel est prêt.

---

## 2) Mise en route sur matériel (checklist simple)

Pour une intégration progressive et claire:

1. **Écran**: vérifier que l'interface s'affiche.
2. **GPS**: connecter le module (UART/USB), puis vérifier que les données remontent.
3. **Bluetooth**: activer le service Bluetooth, tester l'appairage et le contrôle média.
4. **Caméra**: vérifier la réception du flux UDP/JPEG.
5. **IMU**: brancher le capteur I2C (MPU9250) et valider les mesures.

---

## 3) Câblage matériel (Raspberry Pi, exemple recommandé)

### GPS (UART)

- GPS **TX** → Raspberry Pi **GPIO15 / RXD** (pin physique 10)
- GPS **RX** → Raspberry Pi **GPIO14 / TXD** (pin physique 8)
- GPS **VCC** → **5V** (ou 3.3V selon module)
- GPS **GND** → **GND**

### IMU MPU9250 (I2C)

- MPU9250 **SDA** → Raspberry Pi **GPIO2 / SDA1** (pin 3)
- MPU9250 **SCL** → Raspberry Pi **GPIO3 / SCL1** (pin 5)
- MPU9250 **VCC** → **3.3V**
- MPU9250 **GND** → **GND**
- Adresse I2C attendue côté code: **0x68** (`/dev/i2c-1`)

### Caméra

- Le module attend un flux **JPEG sur UDP** (source locale ou distante).

> ⚠️ Important: vérifier le niveau logique (3.3V/5V) de vos modules avant câblage.

---

## 4) Configuration utile

- **Cartographie**: définir `MAPBOX_API_KEY`.
- **Home Assistant**: adapter l'URL dans `homeassistant.cpp`.

---

## 5) Structure du dépôt (lecture guidée)

```text
.
├── README.md                # Point d'entrée: installation et usage
├── InterfaceGPS.pro         # Modules Qt et build qmake
├── docs/                    # Documentation d'architecture, build et matériel
├── tests/                   # Tests Qt (unitaires / UI)
├── scripts/                 # Scripts tests + Doxygen
└── *.cpp / *.h / *.ui / *.qml
```

---

## 6) Documentation (ordre recommandé)

1. **Guide principal**: `README.md`
2. **Démarrage détaillé**: [`docs/build.md`](docs/build.md)
3. **Matériel cible**: [`docs/hardware.md`](docs/hardware.md)
4. **Architecture**: [`docs/architecture.md`](docs/architecture.md)
5. **Navigation**: [`docs/navigation.md`](docs/navigation.md)
6. **Index docs**: [`docs/index.md`](docs/index.md)

---


## 7) Documentation API (Doxygen)

```bash
bash scripts/run_doxygen.sh
```

Sortie locale: `doc_output/html/index.html`

---
