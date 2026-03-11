import QtQuick
import QtTest

TestCase {
    name: "MapQmlTest"

    property var mapRoot: null

    function initTestCase() {
        const component = Qt.createComponent("../../map.qml")
        compare(component.status, Component.Ready, component.errorString())
        mapRoot = component.createObject(null)
        verify(mapRoot !== null)
    }

    function cleanupTestCase() {
        if (mapRoot) {
            mapRoot.destroy()
            mapRoot = null
        }
    }

    function test_formatWazeDistance_roundingRules() {
        compare(mapRoot.formatWazeDistance(10), "0 m")
        compare(mapRoot.formatWazeDistance(271), "270 m")
        compare(mapRoot.formatWazeDistance(640), "650 m")
        compare(mapRoot.formatWazeDistance(1540), "1.5 km")
    }

    function test_mapboxModifierToDirection_mapping() {
        compare(mapRoot.mapboxModifierToDirection("arrive", ""), 0)
        compare(mapRoot.mapboxModifierToDirection("roundabout", "right"), 100)
        compare(mapRoot.mapboxModifierToDirection("turn", "slight right"), 3)
        compare(mapRoot.mapboxModifierToDirection("turn", "right"), 4)
        compare(mapRoot.mapboxModifierToDirection("turn", "uturn"), 6)
        compare(mapRoot.mapboxModifierToDirection("turn", "left"), 9)
        compare(mapRoot.mapboxModifierToDirection("turn", "slight left"), 10)
    }

    function test_getDirectionIconPath_returnsExpectedSvg() {
        compare(mapRoot.getDirectionIconPath(0), "qrc:/icons/dir_arrival.svg")
        compare(mapRoot.getDirectionIconPath(4), "qrc:/icons/dir_right.svg")
        compare(mapRoot.getDirectionIconPath(100), "qrc:/icons/rond_point.svg")
        compare(mapRoot.getDirectionIconPath(999), "qrc:/icons/dir_straight.svg")
    }

    function test_recenterMap_reenablesAutoFollowAndSpeedZoom() {
        mapRoot.autoFollow = false
        mapRoot.enableSpeedZoom = false
        mapRoot.carLat = 43.2965
        mapRoot.carLon = 5.3698

        mapRoot.recenterMap()

        compare(mapRoot.autoFollow, true)
        compare(mapRoot.enableSpeedZoom, true)
    }

    function test_onCarZoomChanged_disablesSpeedZoomWhenManual() {
        mapRoot.enableSpeedZoom = true
        mapRoot.internalZoomChange = false

        mapRoot.carZoom = mapRoot.carZoom + 1

        compare(mapRoot.enableSpeedZoom, false)
    }
}
