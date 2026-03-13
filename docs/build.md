# Guide rapide

## Prérequis

- Linux (Ubuntu / Raspberry Pi OS recommandé)
- Compilateur C++17, `make`, `qmake6`

## Installation des dépendances

```bash
bash scripts/install_dependencies.sh
```

## Compiler et lancer

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
./InterfaceGPS
```

## Configuration minimale

- Définir `MAPBOX_API_KEY` pour la carte
- Adapter l’URL Home Assistant dans `homeassistant.cpp` si nécessaire

## Documentation Doxygen

```bash
bash scripts/run_doxygen.sh
```

Sortie locale : `doc_output/html/index.html`
