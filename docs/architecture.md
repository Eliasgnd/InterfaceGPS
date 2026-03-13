# Architecture logicielle

## Vue d’ensemble

InterfaceGPS est conçu comme une application Qt/C++ modulaire pour un contexte embarqué.
L’objectif est de séparer clairement l’**interface utilisateur**, la **logique applicative**, les **sources de données** et les **intégrations matérielles/réseau**.

L’architecture s’appuie principalement sur les signaux/slots Qt, un modèle de données partagé (`TelemetryData`) et des composants spécialisés par domaine (navigation, média, caméra, capteurs, connectivité).

## Grands blocs du projet

- **Couche UI**
  - `MainWindow` (orchestration globale)
  - pages UI (navigation, média, caméra, paramètres)
  - vues cartographiques QML
- **Logique métier**
  - coordination des modules
  - gestion de l’état applicatif et des transitions de pages
- **Sources de données**
  - GPS/télémétrie
  - capteurs inertiels (MPU9250 selon matériel)
  - informations multimédia et services connectés
- **Intégrations système / matériel**
  - série/I2C (selon cible)
  - réseau local/UDP
  - DBus/MPRIS/Home Assistant selon configuration

## Flux principaux

1. **Démarrage**
   - `main.cpp` initialise les objets cœur, les sources de données et la fenêtre principale.
2. **Flux UI ↔ logique**
   - `MainWindow` pilote l’affichage des pages et centralise les interactions haut niveau.
3. **Flux données capteurs → modèle partagé**
   - Les sources GPS/capteurs mettent à jour `TelemetryData`.
4. **Flux modèle partagé → UI**
   - Les pages consomment les signaux de changement pour rafraîchir l’affichage en temps réel.
5. **Flux services connectés**
   - Les intégrations réseau/multimédia exposent commandes et états à l’interface.

## Schéma d’architecture (texte)

```text
+-------------------------+         +-------------------------+
|         UI Layer        |         |   External Integrations |
|  MainWindow + UI pages  |<------->| Bluetooth / HomeAssist. |
|  Navigation / Media /   |         | DBus / Network services |
|  Camera / Settings      |         +-------------------------+
+------------+------------+
             |
             v
+-------------------------+
|  Application Core       |
|  State orchestration    |
|  Signals / Slots        |
+------------+------------+
             |
             v
+-------------------------+         +-------------------------+
| Shared Data Model       |<--------| Data Sources            |
| TelemetryData           |         | GPS / MPU9250 / Camera  |
+------------+------------+         +------------+------------+
             |                                   |
             +-------------------+---------------+
                                 v
                       +-------------------------+
                       | Hardware / OS Interfaces|
                       | Serial / I2C / UDP      |
                       +-------------------------+
```

## Principes de conception

- **Modularité** : chaque page et chaque source de données a un rôle identifié.
- **Couplage faible** : échanges via signaux/slots et modèles partagés.
- **Lisibilité opérationnelle** : séparation claire entre UI, logique, données, intégration.
- **Évolutivité** : ajout progressif de nouvelles sources capteurs/services sans refonte globale.
- **Pragmatisme embarqué** : comportement adaptable selon les périphériques réellement disponibles.

## Voir aussi

- [Page d’accueil de la documentation](mainpage.md)
- [Navigation et cartographie](navigation.md)
- [Matériel cible](hardware.md)
