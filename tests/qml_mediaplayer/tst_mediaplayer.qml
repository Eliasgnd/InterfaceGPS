import QtQuick
import QtTest

TestCase {
    name: "MediaPlayerQmlTest"

    property var host: null
    property var player: null
    property var bt: null

    function initTestCase() {
        const component = Qt.createComponent("MediaPlayerUnderTest.qml")
        compare(component.status, Component.Ready, component.errorString())

        host = component.createObject(null)
        verify(host !== null)

        player = host.player
        bt = host.mockBluetoothManager
        verify(player !== null)
        verify(bt !== null)
    }

    function cleanupTestCase() {
        if (host) {
            host.destroy()
            host = null
        }
    }

    function test_formatTime_and_clamp_helpers() {
        compare(player.formatTime(0), "00:00")
        compare(player.formatTime(61000), "01:01")

        compare(player.clamp(5, 0, 10), 5)
        compare(player.clamp(-5, 0, 10), 0)
        compare(player.clamp(99, 0, 10), 10)
    }

    function test_compactMode_propertyToggle() {
        compare(player.isCompactMode, false)
        player.isCompactMode = true
        compare(player.isCompactMode, true)
    }

    function test_connections_syncUiPositionFromBluetoothSignals() {
        bt.positionMs = 42000
        bt.positionChanged()
        compare(player.uiPositionMs, 42000)

        bt.positionMs = 15000
        bt.metadataChanged()
        compare(player.uiPositionMs, 15000)
    }

    function test_statusChanged_syncsWhenPlayingOnly() {
        player.uiPositionMs = 2000
        bt.positionMs = 9000
        bt.isPlaying = false
        bt.statusChanged()
        compare(player.uiPositionMs, 2000)

        bt.isPlaying = true
        bt.statusChanged()
        compare(player.uiPositionMs, 9000)
    }
}
