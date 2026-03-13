# InterfaceGPS — Documentation technique

Bienvenue dans la documentation Doxygen d'**InterfaceGPS**, une application **Qt/C++** orientée interface embarquée pour la navigation, la télémétrie et les usages multimédias/capteurs.

Cette documentation combine :
- des **guides d'architecture et d'intégration** (Markdown),
- une **documentation API C++** générée automatiquement,
- des **diagrammes UML/classes** produits par Graphviz.

## Fonctionnalités principales

- Interface utilisateur multi-pages pilotée par `MainWindow`.
- Expérience navigation/cartographie avec intégration QML.
- Gestion de la télémétrie GPS et de l'état de déplacement.
- Intégration caméra pour affichage de flux vidéo.
- Prise en charge capteurs inertiels (ex. MPU9250 selon la cible matérielle).
- Intégrations réseau et écosystème domotique (ex. Home Assistant).
- Fonctions multimédia et connectivité (Bluetooth/MPRIS selon l'environnement).

## Architecture logicielle

Le projet suit une architecture modulaire adaptée à l'embarqué :
- **UI Qt Widgets/QML** pour les écrans et interactions,
- **logique métier** pour orchestrer l'état applicatif,
- **sources de données** (GPS, capteurs, services réseau),
- **couche matériel/intégration** selon la plateforme cible.

Pour le détail, voir la page [Architecture logicielle](architecture.md).

## Modules principaux

- `MainWindow` (orchestration UI)
- pages UI (`NavigationPage`, `MediaPage`, `CameraPage`, `SettingsPage`)
- navigation / cartographie
- télémétrie GPS (`TelemetryData`, `GpsTelemetrySource`)
- caméra
- capteurs / MPU9250
- intégrations réseau / Home Assistant

## Build rapide

1. Installer les dépendances projet (Qt, Doxygen, Graphviz).
2. Générer la documentation :
   ```bash
   bash scripts/run_doxygen.sh
   ```
3. Ouvrir ensuite : `doc_output/html/index.html`

Guide détaillé : [Build & exécution](build.md).

## Navigation dans la documentation

- [Architecture logicielle](architecture.md)
- [Navigation et cartographie](navigation.md)
- [Matériel cible et câblage](hardware.md)
- [Guide de build](build.md)
- [Documentation du dossier docs](README.md)

## Voir aussi

- README principal du dépôt : [`README.md`](../README.md)
- Workflow de publication GitHub Pages : [`.github/workflows/doxygen-pages.yml`](../.github/workflows/doxygen-pages.yml)
