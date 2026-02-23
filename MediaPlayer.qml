/**
 * @file MediaPlayer.qml
 * @brief Rôle architectural : Interface média QML consommée par MediaPage.
 * @details Responsabilités : Rendre visuellement les métadonnées de lecture (titre, artiste),
 * animer les contrôles de transport (Play/Pause) et basculer élégamment
 * entre un mode compact (Split-Screen) et normal (Plein écran).
 * Dépendances principales : Composant C++ 'bluetoothManager' injecté via le contexte, Qt Quick Controls 2.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: root

    // Vue purement déclarative: l'état fonctionnel provient de BluetoothManager exposé depuis C++.
    width: 800
    height: 480

    /** @brief Couleur d'accentuation globale du lecteur (Bleu néon). */
    readonly property color accentColor: "#2a75ff"

    /** @brief Indique si l'application est en mode écran partagé (masque le disque vinyle). */
    property bool isCompactMode: false

    /** @brief Position de lecture locale à l'UI, interpolée pour être fluide. */
    property int uiPositionMs: 0

    // --- FONCTIONS UTILITAIRES ---

    /**
     * @brief Convertit des millisecondes en texte formaté MM:SS.
     * @param ms Durée en millisecondes.
     */
    function formatTime(ms) {
        if (ms <= 0 || isNaN(ms)) return "00:00"
        let totalSec = Math.floor(ms / 1000)
        let m = Math.floor(totalSec / 60)
        let s = totalSec % 60
        return (m < 10 ? "0"+m : m) + ":" + (s < 10 ? "0"+s : s)
    }

    /**
     * @brief Contraint une valeur entre un minimum et un maximum.
     */
    function clamp(val, minV, maxV) {
        return Math.max(minV, Math.min(maxV, val))
    }

    // --- SYNCHRONISATION DBUS ---
    // Les signaux DBus provenant des smartphones peuvent être irréguliers selon le lecteur (Spotify, Apple Music).
    // Ces connexions garantissent que la barre de lecture se recale sur la vraie position serveur.
    Connections {
        target: bluetoothManager
        function onPositionChanged() { root.uiPositionMs = bluetoothManager.positionMs }
        function onMetadataChanged() { root.uiPositionMs = bluetoothManager.positionMs }
        function onStatusChanged() {
             if(bluetoothManager.isPlaying) root.uiPositionMs = bluetoothManager.positionMs
        }
    }

    // Le timer simule une progression locale (interpolation) fluide seconde par seconde
    // pour éviter que la barre de progression ne saccade entre deux mises à jour DBus.
    Timer {
        id: uiProgressTimer
        interval: 1000
        repeat: true
        running: bluetoothManager.isPlaying
        onTriggered: {
            if (bluetoothManager.durationMs > 0) {
                root.uiPositionMs = root.clamp(root.uiPositionMs + 1000, 0, bluetoothManager.durationMs)
            }
        }
    }

    Component.onCompleted: root.uiPositionMs = bluetoothManager.positionMs

    // --- INTERFACE VISUELLE ---

    // Fond dégradé sombre
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0f1115" }
            GradientStop { position: 1.0; color: "#16181d" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 40
        anchors.bottomMargin: 20
        spacing: 30

        // 1. Zone du Disque Vinyle Animé (Masquée en mode Compact)
        Item {
            visible: !root.isCompactMode
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter

            // Halo lumineux d'arrière-plan
            Rectangle {
                width: 340; height: 340
                anchors.centerIn: parent
                color: root.accentColor
                radius: 170
                opacity: 0.15
                layer.enabled: true
                layer.effect: MultiEffect { blurEnabled: true; blurMax: 40; blur: 1.0 }
            }

            // Graphisme du Vinyle
            Item {
                width: 300; height: 300
                anchors.centerIn: parent
                Rectangle {
                    anchors.fill: parent; radius: width / 2; color: "#050505"; border.color: "#222"; border.width: 1
                    Rectangle { width: 280; height: 280; radius: 140; anchors.centerIn: parent; color: "transparent"; border.color: "#1a1a1a"; border.width: 2 }
                    Rectangle { width: 220; height: 220; radius: 110; anchors.centerIn: parent; color: "transparent"; border.color: "#1a1a1a"; border.width: 2 }
                    Rectangle { width: 160; height: 160; radius: 80;  anchors.centerIn: parent; color: "transparent"; border.color: "#1a1a1a"; border.width: 2 }

                    // Centre du disque (Label)
                    Rectangle {
                        width: 110; height: 110; radius: 55; anchors.centerIn: parent
                        gradient: Gradient { GradientStop { position: 0.0; color: "#003366" } GradientStop { position: 1.0; color: root.accentColor } }
                        Text { anchors.centerIn: parent; text: "♫"; font.pixelSize: 45; color: "white" }
                    }

                    // Animation : Le disque tourne uniquement si la musique est sur Play
                    RotationAnimation on rotation {
                        from: 0; to: 360; duration: 8000; loops: Animation.Infinite; running: true; paused: !bluetoothManager.isPlaying
                    }
                }
            }
        }

        // 2. Zone des Contrôles et Informations
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            // 2.1 Informations Texte (Titre, Artiste, Album)
            ColumnLayout {
                Layout.fillWidth: true; spacing: 6
                Label { Layout.fillWidth: true; text: bluetoothManager.title; font.pixelSize: 38; font.weight: Font.Bold; color: "white"; elide: Text.ElideRight }
                Label { Layout.fillWidth: true; text: bluetoothManager.artist; font.pixelSize: 22; font.weight: Font.Medium; color: "#8892a0"; elide: Text.ElideRight }
                Label { Layout.fillWidth: true; visible: bluetoothManager.album && bluetoothManager.album.length > 0; text: bluetoothManager.album; font.pixelSize: 16; color: "#6f7886"; elide: Text.ElideRight }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

            // 2.2 Barre de progression du temps
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                ProgressBar {
                    id: timeBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: 8

                    from: 0
                    to: Math.max(1, bluetoothManager.durationMs)
                    value: bluetoothManager.durationMs > 0 ? root.uiPositionMs : 0

                    background: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 8
                        color: "#2a2f3a"
                        radius: 4
                    }

                    contentItem: Item {
                        implicitWidth: 200
                        implicitHeight: 8
                        Rectangle {
                            width: timeBar.visualPosition * parent.width
                            height: parent.height
                            radius: 4
                            color: root.accentColor
                        }
                    }
                }

                // Affichage textuel du chronomètre (Actuel --- Total)
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: bluetoothManager.durationMs > 0 ? formatTime(root.uiPositionMs) : "--:--"
                        color: "#8892a0"; font.pixelSize: 14
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: bluetoothManager.durationMs > 0 ? formatTime(bluetoothManager.durationMs) : "--:--"
                        color: "#8892a0"; font.pixelSize: 14
                    }
                }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

            // 2.3 Boutons de contrôle (Previous, Play/Pause, Next)
            RowLayout {
                Layout.fillWidth: true; Layout.alignment: Qt.AlignCenter; spacing: 0
                Item { Layout.fillWidth: true }

                // Bouton Précédent
                RoundButton {
                    Layout.preferredWidth: 70; Layout.preferredHeight: 70
                    background: Item {}
                    contentItem: Shape {
                        anchors.centerIn: parent; width: 28; height: 28
                        ShapePath { strokeWidth: 0; fillColor: "white"; startX: 26; startY: 0; PathLine { x: 26; y: 28 } PathLine { x: 4;  y: 14 } PathLine { x: 26; y: 0 } PathMove { x: 2; y: 0 } PathLine { x: 2; y: 28 } PathLine { x: 0; y: 28 } PathLine { x: 0; y: 0 } }
                    }
                    onClicked: bluetoothManager.previous()
                }

                Item { Layout.fillWidth: true }

                // Bouton central Play/Pause (L'icône change dynamiquement)
                RoundButton {
                    Layout.preferredWidth: 90; Layout.preferredHeight: 90
                    background: Rectangle { radius: 45; color: "white" }
                    contentItem: Item {
                        anchors.fill: parent
                        // Icône Pause (2 barres)
                        Row { anchors.centerIn: parent; spacing: 7; visible: bluetoothManager.isPlaying; Rectangle { width: 7; height: 28; color: "black"; radius: 1 } Rectangle { width: 7; height: 28; color: "black"; radius: 1 } }
                        // Icône Play (Triangle)
                        Shape {
                            anchors.centerIn: parent; anchors.horizontalCenterOffset: 3; visible: !bluetoothManager.isPlaying
                            ShapePath { strokeWidth: 0; fillColor: "black"; startX: 0; startY: 0; PathLine { x: 0;  y: 28 } PathLine { x: 24; y: 14 } PathLine { x: 0;  y: 0 } }
                        }
                    }
                    onClicked: bluetoothManager.togglePlay()
                }

                Item { Layout.fillWidth: true }

                // Bouton Suivant
                RoundButton {
                    Layout.preferredWidth: 70; Layout.preferredHeight: 70
                    background: Item {}
                    contentItem: Shape {
                        anchors.centerIn: parent; width: 28; height: 28
                        ShapePath { strokeWidth: 0; fillColor: "white"; startX: 2; startY: 0; PathLine { x: 2;  y: 28 } PathLine { x: 24; y: 14 } PathLine { x: 2;  y: 0 } PathMove { x: 26; y: 0 } PathLine { x: 26; y: 28 } PathLine { x: 28; y: 28 } PathLine { x: 28; y: 0 } }
                    }
                    onClicked: bluetoothManager.next()
                }

                Item { Layout.fillWidth: true }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

            // 2.4 Contrôle du volume (Slider)
            RowLayout {
                Layout.fillWidth: true; spacing: 20
                Image { source: "qrc:/icons/volume_down.svg"; sourceSize.width: 24; sourceSize.height: 24; opacity: 0.7 }
                Slider {
                    id: volumeSlider
                    Layout.fillWidth: true; Layout.preferredHeight: 50; from: 0; to: 1.0; value: 0.7
                    background: Rectangle { implicitHeight: 4; width: parent.width; height: 4; radius: 2; color: "#404040"; anchors.centerIn: parent; Rectangle { width: volumeSlider.visualPosition * parent.width; height: 4; radius: 2; color: root.accentColor } }
                    handle: Rectangle { x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width); y: parent.height / 2 - height / 2; width: 24; height: 24; radius: 12; color: "white" }
                }
                Image { source: "qrc:/icons/volume_up.svg"; sourceSize.width: 24; sourceSize.height: 24; opacity: 0.7 }
            }
        }
    }
}
