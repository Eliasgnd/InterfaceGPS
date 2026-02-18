import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root
    width: 800; height: 480

    // --- LOGIQUE AUDIO ---
    MediaPlayer {
        id: player
        audioOutput: AudioOutput { volume: volumeSlider.value }
        source: "qrc:/music/demo.mp3"
        property var playlist: ["track1.mp3", "track2.mp3"]
    }

    // --- UI ---

    // 1. Fond sombre uni (assorti au reste de ton interface)
    Rectangle {
        anchors.fill: parent
        color: "#171a21"
    }

    // 2. Contenu principal
    RowLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 40

        // A. REMPLACEMENT DE LA POCHETTE (Pas de JPG nécessaire)
        Rectangle {
            Layout.preferredWidth: 300
            Layout.preferredHeight: 300
            radius: 20
            border.color: "#3d4455"
            border.width: 2

            // Un joli dégradé pour donner du relief
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2d3245" }
                GradientStop { position: 1.0; color: "#171a21" }
            }

            Text {
                anchors.centerIn: parent
                text: "🎵"
                font.pixelSize: 100
            }
        }

        // B. Contrôles (Droite)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 20

            // Titre et Artiste
            Label {
                text: "Titre de la chanson"
                font.pixelSize: 32; font.bold: true; color: "white"
            }
            Label {
                text: "Nom de l'artiste"
                font.pixelSize: 22; color: "#b8c0cc"
            }

            // Barre de progression
            Slider {
                id: progressSlider
                Layout.fillWidth: true
                from: 0
                to: player.duration > 0 ? player.duration : 1
                value: player.position
                onMoved: player.setPosition(value)
            }

            // Boutons de contrôle
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 40

                RoundButton {
                    text: "⏮"
                    font.pixelSize: 30
                    onClicked: console.log("Précédent")
                }

                RoundButton {
                    text: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                    font.pixelSize: 40
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80
                    onClicked: {
                        if (player.playbackState === MediaPlayer.PlayingState) player.pause()
                        else player.play()
                    }
                }

                RoundButton {
                    text: "⏭"
                    font.pixelSize: 30
                    onClicked: console.log("Suivant")
                }
            }

            // Volume (On garde tes SVG ici)
             RowLayout {
                Image {
                    source: "qrc:/icons/volume_down.svg"
                    sourceSize.width: 24; sourceSize.height: 24
                }

                Slider {
                    id: volumeSlider
                    Layout.fillWidth: true
                    from: 0; to: 1.0; value: 0.7
                }

                Image {
                    source: "qrc:/icons/volume_up.svg"
                    sourceSize.width: 24; sourceSize.height: 24
                }
            }
        }
    }
}
