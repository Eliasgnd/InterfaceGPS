import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// import QtMultimedia // Plus besoin du module multimédia local
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: root
    width: 800; height: 480

    // --- ÉTATS ---
    property bool isShuffle: false
    property bool isRepeat: false
    property bool isPlaylistOpen: false
    readonly property color accentColor: "#2a75ff"

    // --- FONCTIONS ---
    function formatTime(ms) {
        if (ms <= 0 || isNaN(ms)) return "00:00"
        let totalSec = Math.floor(ms / 1000)
        let m = Math.floor(totalSec / 60)
        let s = totalSec % 60
        return (m < 10 ? "0"+m : m) + ":" + (s < 10 ? "0"+s : s)
    }

    // --- AUDIO ---
    // L'objet MediaPlayer a été supprimé car c'est le téléphone qui gère l'audio.
    // Nous utilisons maintenant l'objet global "bluetoothManager" injecté depuis le C++.

    // --- FOND D'ÉCRAN ---
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0f1115" }
            GradientStop { position: 1.0; color: "#16181d" }
        }
    }

    // --- LAYOUT PRINCIPAL ---
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // === ZONE LECTEUR ===
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 40
                anchors.bottomMargin: 20
                spacing: 30

                // --- 1. LE DISQUE ---
                Item {
                    Layout.preferredWidth: 320
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignVCenter

                    // Halo Bleu
                    Rectangle {
                        width: 340; height: 340
                        anchors.centerIn: parent
                        color: root.accentColor
                        radius: 170
                        opacity: 0.15
                        layer.enabled: true
                        layer.effect: MultiEffect { blurEnabled: true; blurMax: 40; blur: 1.0 }
                    }

                    // Vinyle
                    Item {
                        width: 300; height: 300
                        anchors.centerIn: parent
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: "#050505"
                            border.color: "#222"
                            border.width: 1
                            // Sillons
                            Rectangle { width: 280; height: 280; radius: 140; anchors.centerIn: parent; color: "transparent"; border.color: "#1a1a1a"; border.width: 2 }
                            Rectangle { width: 220; height: 220; radius: 110; anchors.centerIn: parent; color: "transparent"; border.color: "#1a1a1a"; border.width: 2 }
                            Rectangle { width: 160; height: 160; radius: 80; anchors.centerIn: parent; color: "transparent"; border.color: "#1a1a1a"; border.width: 2 }
                            // Label
                            Rectangle {
                                width: 110; height: 110; radius: 55
                                anchors.centerIn: parent
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#003366" }
                                    GradientStop { position: 1.0; color: "#2a75ff" }
                                }
                                Text { anchors.centerIn: parent; text: "♫"; font.pixelSize: 45; color: "white" }
                            }
                            // Animation (Tourne si bluetoothManager.isPlaying est vrai)
                            RotationAnimation on rotation {
                                from: 0; to: 360; duration: 8000; loops: Animation.Infinite
                                running: true;
                                paused: !bluetoothManager.isPlaying
                            }
                        }
                    }
                }

                // --- 2. LES CONTRÔLES ---
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    // Titre
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        Label {
                            Layout.fillWidth: true
                            // MODIFICATION : Liaison avec le C++
                            text: bluetoothManager.title
                            font.pixelSize: 38; font.weight: Font.Bold; color: "white"
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            // MODIFICATION : Liaison avec le C++
                            text: bluetoothManager.artist
                            font.pixelSize: 22; font.weight: Font.Medium; color: "#8892a0"
                        }
                    }

                    Item { Layout.fillHeight: true; Layout.minimumHeight: 20 }

                    // --- BARRE DE PROGRESSION ---
                    // Note: La progression Bluetooth est complexe à récupérer.
                    // Ici, on met des valeurs par défaut pour éviter les erreurs.
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Slider {
                            id: progressSlider
                            Layout.fillWidth: true
                            Layout.preferredHeight: 60
                            enabled: false // Désactivé pour le mode Bluetooth simple

                            from: 0
                            to: 1
                            value: 0

                            background: Rectangle {
                                x: progressSlider.leftPadding
                                y: parent.height / 2 - height / 2
                                width: progressSlider.availableWidth
                                height: 8
                                radius: 4
                                color: "#2a2f3a"

                                Rectangle {
                                    width: progressSlider.visualPosition * parent.width
                                    height: parent.height
                                    color: root.accentColor
                                    radius: 4
                                }
                            }

                            handle: Rectangle {
                                x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                                y: parent.height / 2 - height / 2
                                width: 34; height: 34; radius: 17
                                color: "white"
                                layer.enabled: true
                                layer.effect: MultiEffect { shadowEnabled: true; shadowColor: "black"; shadowBlur: 10 }
                            }
                        }

                        // Temps
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: "--:--"; color: "#8892a0"; font.pixelSize: 14 }
                            Item { Layout.fillWidth: true }
                            Label { text: "--:--"; color: "#8892a0"; font.pixelSize: 14 }
                        }
                    }

                    Item { Layout.fillHeight: true; Layout.minimumHeight: 20 }

                    // BOUTONS DE CONTRÔLE
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        spacing: 0

                        // Shuffle
                        Item { Layout.fillWidth: true }
                        RoundButton {
                            Layout.preferredWidth: 60; Layout.preferredHeight: 60
                            background: Item{}
                            contentItem: Shape {
                                anchors.centerIn: parent; width: 24; height: 24
                                ShapePath {
                                    strokeWidth: 2; strokeColor: root.isShuffle ? root.accentColor : "#666"
                                    fillColor: "transparent"; capStyle: ShapePath.RoundCap
                                    startX: 0; startY: 18; PathCubic { control1X: 10; control1Y: 18; control2X: 14; control2Y: 6; x: 24; y: 6 }
                                    PathMove { x: 0; y: 6 } PathCubic { control1X: 10; control1Y: 6; control2X: 14; control2Y: 18; x: 24; y: 18 }
                                    PathMove { x: 20; y: 2 } PathLine { x: 24; y: 6 } PathLine { x: 20; y: 10 }
                                    PathMove { x: 20; y: 14 } PathLine { x: 24; y: 18 } PathLine { x: 20; y: 22 }
                                }
                            }
                            layer.enabled: root.isShuffle
                            layer.effect: MultiEffect { shadowEnabled: true; shadowColor: root.accentColor; shadowBlur: 10 }
                            onClicked: root.isShuffle = !root.isShuffle
                        }

                        // Prev
                        Item { Layout.fillWidth: true }
                        RoundButton {
                            Layout.preferredWidth: 70; Layout.preferredHeight: 70
                            background: Item{}
                            contentItem: Shape {
                                anchors.centerIn: parent; width: 28; height: 28
                                ShapePath {
                                    strokeWidth: 0; fillColor: "white"
                                    startX: 26; startY: 0; PathLine { x: 26; y: 28 } PathLine { x: 4; y: 14 } PathLine { x: 26; y: 0 }
                                    PathMove { x: 2; y: 0 } PathLine { x: 2; y: 28 } PathLine { x: 0; y: 28 } PathLine { x: 0; y: 0 }
                                }
                            }
                            // MODIFICATION : Appel C++
                            onClicked: bluetoothManager.previous()
                        }

                        // PLAY / PAUSE
                        Item { Layout.fillWidth: true }
                        RoundButton {
                            Layout.preferredWidth: 90; Layout.preferredHeight: 90
                            background: Rectangle { radius: 45; color: "white" }
                            contentItem: Item {
                                anchors.fill: parent
                                Row {
                                    anchors.centerIn: parent; spacing: 7
                                    // Visible si lecture en cours
                                    visible: bluetoothManager.isPlaying
                                    Rectangle { width: 7; height: 28; color: "black"; radius: 1 }
                                    Rectangle { width: 7; height: 28; color: "black"; radius: 1 }
                                }
                                Shape {
                                    anchors.centerIn: parent; anchors.horizontalCenterOffset: 3
                                    // Visible si pause
                                    visible: !bluetoothManager.isPlaying
                                    ShapePath {
                                        strokeWidth: 0; fillColor: "black"
                                        startX: 0; startY: 0; PathLine { x: 0; y: 28 } PathLine { x: 24; y: 14 } PathLine { x: 0; y: 0 }
                                    }
                                }
                            }
                            // MODIFICATION : Appel C++
                            onClicked: bluetoothManager.togglePlay()
                        }

                        // Next
                        Item { Layout.fillWidth: true }
                        RoundButton {
                            Layout.preferredWidth: 70; Layout.preferredHeight: 70
                            background: Item{}
                            contentItem: Shape {
                                anchors.centerIn: parent; width: 28; height: 28
                                ShapePath {
                                    strokeWidth: 0; fillColor: "white"
                                    startX: 2; startY: 0; PathLine { x: 2; y: 28 } PathLine { x: 24; y: 14 } PathLine { x: 2; y: 0 }
                                    PathMove { x: 26; y: 0 } PathLine { x: 26; y: 28 } PathLine { x: 28; y: 28 } PathLine { x: 28; y: 0 }
                                }
                            }
                            // MODIFICATION : Appel C++
                            onClicked: bluetoothManager.next()
                        }

                        // Repeat
                        Item { Layout.fillWidth: true }
                        RoundButton {
                            Layout.preferredWidth: 60; Layout.preferredHeight: 60
                            background: Item{}
                            contentItem: Shape {
                                anchors.centerIn: parent; width: 24; height: 24
                                ShapePath {
                                    strokeWidth: 2
                                    strokeColor: root.isRepeat ? root.accentColor : "#666"
                                    fillColor: "transparent"; capStyle: ShapePath.RoundCap
                                    startX: 2; startY: 12;
                                    PathArc { x: 22; y: 12; radiusX: 10; radiusY: 10; useLargeArc: true }
                                    PathMove { x: 22; y: 8 } PathLine { x: 22; y: 12 } PathLine { x: 18; y: 12 }
                                }
                            }
                            layer.enabled: root.isRepeat
                            layer.effect: MultiEffect { shadowEnabled: true; shadowColor: root.accentColor; shadowBlur: 10 }
                            onClicked: root.isRepeat = !root.isRepeat
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Item { Layout.fillHeight: true; Layout.minimumHeight: 30 }

                    // VOLUME & PLAYLIST TOGGLE
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 20
                        Image { source: "qrc:/icons/volume_down.svg"; sourceSize.width: 24; sourceSize.height: 24; opacity: 0.7 }
                        Slider {
                            id: volumeSlider
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            from: 0; to: 1.0; value: 0.7

                            background: Rectangle {
                                implicitHeight: 4; width: parent.width; height: 4; radius: 2; color: "#404040"
                                anchors.centerIn: parent
                                Rectangle { width: volumeSlider.visualPosition * parent.width; height: 4; radius: 2; color: root.accentColor }
                            }
                            handle: Rectangle {
                                x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                                y: parent.height / 2 - height / 2
                                width: 24; height: 24; radius: 12; color: "white"
                            }
                        }
                        // Bouton Playlist
                        RoundButton {
                            Layout.preferredWidth: 50; Layout.preferredHeight: 50
                            background: Rectangle { color: root.isPlaylistOpen ? "#333" : "transparent"; radius: 8 }
                            contentItem: Shape {
                                anchors.centerIn: parent; width: 24; height: 24
                                ShapePath {
                                    strokeWidth: 2; strokeColor: root.isPlaylistOpen ? root.accentColor : "white"; capStyle: ShapePath.RoundCap
                                    startX: 0; startY: 6; PathLine { x: 24; y: 6 }
                                    PathMove { x: 0; y: 12 } PathLine { x: 24; y: 12 }
                                    PathMove { x: 0; y: 18 } PathLine { x: 16; y: 18 }
                                    PathMove { x: 19; y: 16 } PathLine { x: 19; y: 20 } PathLine { x: 23; y: 18 } PathLine { x: 19; y: 16 }
                                }
                            }
                            onClicked: root.isPlaylistOpen = !root.isPlaylistOpen
                        }
                    }
                }
            }
        }

        // === PANNEAU LATÉRAL (Playlist - Statique pour l'instant) ===
        Rectangle {
            id: playlistPanel
            Layout.preferredWidth: root.isPlaylistOpen ? 300 : 0
            Layout.fillHeight: true
            Behavior on Layout.preferredWidth { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }

            clip: true
            color: "#18181a"
            Rectangle { width: 1; height: parent.height; color: "#333"; anchors.left: parent.left }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                visible: playlistPanel.width > 50
                spacing: 20
                Label { text: "File d'attente"; font.pixelSize: 22; font.bold: true; color: "white" }

                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true
                    model: ListModel {
                        ListElement { title: "Bluetooth Audio"; artist: "Stream"; playing: true }
                    }
                    delegate: Item {
                        width: ListView.view.width; height: 60
                        RowLayout {
                            anchors.fill: parent; spacing: 10
                            Item {
                                Layout.preferredWidth: 15; Layout.preferredHeight: 15
                                visible: playing
                                Row {
                                    anchors.centerIn: parent; spacing: 2
                                    Repeater {
                                        model: 3
                                        Rectangle { width: 3; height: 8 + Math.random()*8; color: root.accentColor; radius: 1 }
                                    }
                                }
                            }
                            Column {
                                Layout.fillWidth: true
                                Text { text: title; color: playing ? root.accentColor : "white"; font.pixelSize: 16; font.bold: true }
                                Text { text: artist; color: "#888"; font.pixelSize: 12 }
                            }
                        }
                        Rectangle { width: parent.width; height: 1; color: "#252525"; anchors.bottom: parent.bottom }
                    }
                }
            }
        }
    }
}
