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

    // Signaux vers le C++
    signal routeInfoUpdated(string distance, string duration)

    Plugin {
        id: mapPlugin
        name: "osm"

        // 1. Mapbox Streets (Style Google Maps)
        PluginParameter {
            name: "osm.mapping.custom.host"
            value: "https://api.mapbox.com/styles/v1/mapbox/streets-v12/tiles/512/%z/%x/%y@2x?access_token=" + mapboxApiKey
        }
        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }

        // 2. Routing (OSRM)
        PluginParameter { name: "osm.routing.host"; value: "http://router.project-osrm.org/route/v1/driving" }

        // 3. CACHE DISQUE (500 Mo) - Pour ne pas recharger la carte à chaque fois
        PluginParameter { name: "osm.mapping.cache.disk.cost_strategy"; value: "bytes" }
        PluginParameter { name: "osm.mapping.cache.disk.size"; value: "500000000" }

        PluginParameter { name: "osm.useragent"; value: "MonAppliGPS_IUT" }
    }

    // --- MODELE RECHERCHE FINALE (Itinéraire) ---
    GeocodeModel {
        id: geocodeModel
        plugin: mapPlugin
        limit: 1
        onLocationsChanged: {
            if (count > 0) {
                var loc = get(0).coordinate;

                routeQuery.clearWaypoints();
                routeQuery.addWaypoint(QtPositioning.coordinate(carLat, carLon));
                routeQuery.addWaypoint(loc);

                // On lance le calcul MANUELLEMENT
                routeModel.update();
            }
        }
    }

    RouteQuery { id: routeQuery }

    RouteModel {
        id: routeModel
        plugin: mapPlugin
        query: routeQuery
        autoUpdate: false // Pas de recalcul auto pour économiser

        onStatusChanged: {
            if (status === RouteModel.Ready && count > 0) {
                var route = get(0);
                map.fitViewport(route.path);
                root.autoFollow = false;

                var d = (route.distance / 1000).toFixed(1) + " km";
                var t = Math.round(route.travelTime / 60) + " min";
                routeInfoUpdated(d, t);
            }
        }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(carLat, carLon)
        zoomLevel: carZoom
        tilt: autoFollow ? 45 : 0
        copyrightsVisible: false

        Component.onCompleted: {
            for (var i = 0; i < supportedMapTypes.length; i++) {
                if (supportedMapTypes[i].style === MapType.CustomMap) activeMapType = supportedMapTypes[i];
            }
        }

        MapItemView {
            model: routeModel
            delegate: MapRoute { route: routeData; line.color: "#1db7ff"; line.width: 12; opacity: 0.9 }
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
                Rectangle { anchors.centerIn: parent; width: 44; height: 44; color: "#00a8ff"; opacity: 0.2; radius: 22 }
                Rectangle {
                    anchors.centerIn: parent; width: 22; height: 30; color: "white"; radius: 4; border.width: 2; border.color: "#171a21"
                    Rectangle { width: 22; height: 22; color: "white"; rotation: 45; y: -8; anchors.horizontalCenter: parent.horizontalCenter; border.width: 2; border.color: "#171a21" }
                    Rectangle { width: 18; height: 18; color: "white"; anchors.centerIn: parent; y: -2 }
                }
                transform: Rotation { origin.x: 30; origin.y: 30; angle: carHeading }
            }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: { root.autoFollow = false; }
            onPositionChanged: (mouse) => mouse.accepted = false
        }
    }

    // Panneau d'info flottant
    Rectangle {
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 20
        width: 180; height: 60; color: "#171a21"; radius: 12; border.color: "#3d4455"; border.width: 1
        visible: routeModel.count > 0
        Column {
            anchors.centerIn: parent
            Text { text: routeModel.count > 0 ? Math.round(routeModel.get(0).travelTime / 60) + " min" : ""; color: "white"; font.pixelSize: 18; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
            Text { text: routeModel.count > 0 ? (routeModel.get(0).distance / 1000).toFixed(1) + " km" : ""; color: "#b8c0cc"; font.pixelSize: 14; anchors.horizontalCenter: parent.horizontalCenter }
        }
    }

    // Fonctions appelées par le C++

    function searchDestination(address) {
        geocodeModel.query = address;
        geocodeModel.update();
    }

    onCarLatChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }
    onCarLonChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }

    function recenterMap() {
        autoFollow = true;
        map.center = QtPositioning.coordinate(carLat, carLon);
        map.zoomLevel = 17;
    }
}
