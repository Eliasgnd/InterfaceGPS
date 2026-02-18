import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: root
    width: 800
    height: 480

    readonly property color accentColor: "#2a75ff"

    function formatTime(ms) {
        if (ms <= 0 || isNaN(ms)) return "00:00"
        let totalSec = Math.floor(ms / 1000)
        let m = Math.floor(totalSec / 60)
        let s = totalSec % 60
        return (m < 10 ? "0"+m : m) + ":" + (s < 10 ? "0"+s : s)
    }

    // --- Fond ---
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

        // =====================
        // 1) Disque / vinyle
        // =====================
        Item {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter

            // Halo
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
                    Rectangle { width: 160; height: 160; radius: 80;  anchors.centerIn: parent; color: "transparent"; border.color: "#1a1a1a"; border.width: 2 }

                    // Label
                    Rectangle {
                        width: 110; height: 110; radius: 55
                        anchors.centerIn: parent
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#003366" }
                            GradientStop { position: 1.0; color: root.accentColor }
                        }
                        Text { anchors.centerIn: parent; text: "♫"; font.pixelSize: 45; color: "white" }
                    }

                    // Rotation
                    RotationAnimation on rotation {
                        from: 0; to: 360
                        duration: 8000
                        loops: Animation.Infinite
                        running: true
                        paused: !bluetoothManager.isPlaying
                    }
                }
            }
        }

        // =====================
        // 2) Infos + contrôles
        // =====================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---- Titre / Artiste / Album ----
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    text: bluetoothManager.title
                    font.pixelSize: 38
                    font.weight: Font.Bold
                    color: "white"
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: bluetoothManager.artist
                    font.pixelSize: 22
                    font.weight: Font.Medium
                    color: "#8892a0"
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    visible: bluetoothManager.album && bluetoothManager.album.length > 0
                    text: bluetoothManager.album
                    font.pixelSize: 16
                    color: "#6f7886"
                    elide: Text.ElideRight
                }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

            // ---- Progression (lecture seule) ----
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Slider {
                    id: progressSlider
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60

                    enabled: bluetoothManager.durationMs > 0
                    from: 0
                    to: Math.max(1, bluetoothManager.durationMs)
                    value: Math.min(bluetoothManager.positionMs, bluetoothManager.durationMs)

                    // Lecture seule (seek bluetooth pas fiable)
                    onPressedChanged: if (pressed) pressed = false

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
                        visible: progressSlider.enabled
                        x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                        y: parent.height / 2 - height / 2
                        width: 34; height: 34; radius: 17
                        color: "white"
                        layer.enabled: true
                        layer.effect: MultiEffect { shadowEnabled: true; shadowColor: "black"; shadowBlur: 10 }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: bluetoothManager.durationMs > 0 ? formatTime(bluetoothManager.positionMs) : "--:--"
                        color: "#8892a0"
                        font.pixelSize: 14
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: bluetoothManager.durationMs > 0 ? formatTime(bluetoothManager.durationMs) : "--:--"
                        color: "#8892a0"
                        font.pixelSize: 14
                    }
                }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

            // ---- Contrôles ----
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter
                spacing: 0

                Item { Layout.fillWidth: true }

                // Prev
                RoundButton {
                    Layout.preferredWidth: 70; Layout.preferredHeight: 70
                    background: Item {}
                    contentItem: Shape {
                        anchors.centerIn: parent; width: 28; height: 28
                        ShapePath {
                            strokeWidth: 0; fillColor: "white"
                            startX: 26; startY: 0
                            PathLine { x: 26; y: 28 }
                            PathLine { x: 4;  y: 14 }
                            PathLine { x: 26; y: 0 }
                            PathMove { x: 2; y: 0 }
                            PathLine { x: 2; y: 28 }
                            PathLine { x: 0; y: 28 }
                            PathLine { x: 0; y: 0 }
                        }
                    }
                    onClicked: bluetoothManager.previous()
                }

                Item { Layout.fillWidth: true }

                // Play / Pause
                RoundButton {
                    Layout.preferredWidth: 90; Layout.preferredHeight: 90
                    background: Rectangle { radius: 45; color: "white" }

                    contentItem: Item {
                        anchors.fill: parent

                        Row {
                            anchors.centerIn: parent
                            spacing: 7
                            visible: bluetoothManager.isPlaying
                            Rectangle { width: 7; height: 28; color: "black"; radius: 1 }
                            Rectangle { width: 7; height: 28; color: "black"; radius: 1 }
                        }

                        Shape {
                            anchors.centerIn: parent
                            anchors.horizontalCenterOffset: 3
                            visible: !bluetoothManager.isPlaying
                            ShapePath {
                                strokeWidth: 0; fillColor: "black"
                                startX: 0; startY: 0
                                PathLine { x: 0;  y: 28 }
                                PathLine { x: 24; y: 14 }
                                PathLine { x: 0;  y: 0 }
                            }
                        }
                    }

                    onClicked: bluetoothManager.togglePlay()
                }

                Item { Layout.fillWidth: true }

                // Next
                RoundButton {
                    Layout.preferredWidth: 70; Layout.preferredHeight: 70
                    background: Item {}
                    contentItem: Shape {
                        anchors.centerIn: parent; width: 28; height: 28
                        ShapePath {
                            strokeWidth: 0; fillColor: "white"
                            startX: 2; startY: 0
                            PathLine { x: 2;  y: 28 }
                            PathLine { x: 24; y: 14 }
                            PathLine { x: 2;  y: 0 }
                            PathMove { x: 26; y: 0 }
                            PathLine { x: 26; y: 28 }
                            PathLine { x: 28; y: 28 }
                            PathLine { x: 28; y: 0 }
                        }
                    }
                    onClicked: bluetoothManager.next()
                }

                Item { Layout.fillWidth: true }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

            // ---- Volume (UI uniquement, pas de contrôle bluetooth universel) ----
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                Image {
                    source: "qrc:/icons/volume_down.svg"
                    sourceSize.width: 24
                    sourceSize.height: 24
                    opacity: 0.7
                }

                Slider {
                    id: volumeSlider
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    from: 0; to: 1.0; value: 0.7

                    background: Rectangle {
                        implicitHeight: 4
                        width: parent.width
                        height: 4
                        radius: 2
                        color: "#404040"
                        anchors.centerIn: parent

                        Rectangle {
                            width: volumeSlider.visualPosition * parent.width
                            height: 4
                            radius: 2
                            color: root.accentColor
                        }
                    }

                    handle: Rectangle {
                        x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                        y: parent.height / 2 - height / 2
                        width: 24; height: 24; radius: 12
                        color: "white"
                    }
                }

                Image {
                    source: "qrc:/icons/volume_up.svg"
                    sourceSize.width: 24
                    sourceSize.height: 24
                    opacity: 0.7
                }
            }
        }
    }
}
