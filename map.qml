import QtQuick
import QtLocation
import QtPositioning

Item {
    id: root
    width: 600; height: 400

    property double carLat: 48.2715
    property double carLon: 4.0645
    property double carZoom: 17
    property double carHeading: 0
    property bool autoFollow: true

    Plugin {
            id: mapPlugin
            name: "osm"

            // 1. On dit à Qt d'aller chercher les images (tuiles) chez Mapbox
            // On utilise la variable 'mapboxApiKey' que le C++ nous envoie
            PluginParameter {
                name: "osm.mapping.custom.host"
                value: "https://api.mapbox.com/styles/v1/mapbox/navigation-night-v1/tiles/256/%z/%x/%y?access_token=" + mapboxApiKey
            }

            // 2. On désactive les cartes OSM standard pour ne pas faire ramer l'appli
            PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }

            // 3. On garde le routage gratuit (OSRM)
            PluginParameter { name: "osm.routing.host"; value: "http://router.project-osrm.org/route/v1/driving" }
        }

    RouteQuery {
        id: routeQuery
        waypoints: [
            { latitude: 48.2715, longitude: 4.0645 },
            { latitude: 48.2975, longitude: 4.0742 }
        ]
    }

    RouteModel {
        id: routeModel
        plugin: mapPlugin
        query: routeQuery
        autoUpdate: true
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(carLat, carLon)
        zoomLevel: carZoom
        tilt: 60
        copyrightsVisible: false

        MapItemView {
            model: routeModel
            delegate: MapRoute {
                route: routeData
                line.color: "#4aa5ff"
                line.width: 10
                opacity: 0.8
            }
        }

        MapQuickItem {
            id: carMarker
            coordinate: QtPositioning.coordinate(carLat, carLon)
            anchorPoint.x: carVisual.width / 2
            anchorPoint.y: carVisual.height / 2
            z: 10
            sourceItem: Item {
                id: carVisual
                width: 60; height: 60
                Rectangle { anchors.centerIn: parent; width: 50; height: 50; color: "#00a8ff"; opacity: 0.3; radius: 25 }
                Rectangle {
                    anchors.centerIn: parent; width: 24; height: 32; color: "white"; radius: 4; border.width: 2; border.color: "#171a21"
                    Rectangle { width: 24; height: 24; color: "white"; rotation: 45; y: -8; anchors.horizontalCenter: parent.horizontalCenter; border.width: 2; border.color: "#171a21" }
                    Rectangle { width: 20; height: 20; color: "white"; anchors.centerIn: parent; y: -2 }
                }
                transform: Rotation { origin.x: 30; origin.y: 30; angle: carHeading }
            }
        }

        // Gestion de l'interaction pour Qt 6
        MouseArea {
            anchors.fill: parent
            onPressed: {
                root.autoFollow = false; // Désactive le suivi quand on touche la carte
            }
            // On laisse passer les événements pour que la Map puisse bouger
            onPositionChanged: (mouse) => mouse.accepted = false
            onWheel: (wheel) => {
                root.autoFollow = false;
                wheel.accepted = false;
            }
        }

        Behavior on center { CoordinateAnimation { duration: 800; easing.type: Easing.Linear } }
    }

    onCarLatChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }
    onCarLonChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }

    function recenterMap() {
        autoFollow = true;
        map.center = QtPositioning.coordinate(carLat, carLon);
    }

    Component.onCompleted: routeModel.update()
}
