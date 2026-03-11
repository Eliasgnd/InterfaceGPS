import QtQuick

Item {
    id: root
    width: 1280
    height: 800

    QtObject {
        id: bluetoothManager

        property int positionMs: 0
        property int durationMs: 180000
        property bool isPlaying: false
        property string title: "Titre test"
        property string artist: "Artiste test"
        property string album: "Album test"

        property int previousCalls: 0
        property int nextCalls: 0
        property int togglePlayCalls: 0

        signal positionChanged()
        signal metadataChanged()
        signal statusChanged()

        function previous() { previousCalls += 1 }
        function next() { nextCalls += 1 }
        function togglePlay() { togglePlayCalls += 1; isPlaying = !isPlaying; statusChanged() }
    }

    property alias player: loader.item
    property alias mockBluetoothManager: bluetoothManager

    Loader {
        id: loader
        anchors.fill: parent
        source: "../../MediaPlayer.qml"
    }
}
