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
    property double carSpeed: 0
    property bool autoFollow: true
    property string nextInstruction: ""

    signal routeInfoUpdated(string distance, string duration)
    signal suggestionsUpdated(string suggestions)

    Plugin {
        id: mapPlugin
        name: "osm"
        PluginParameter {
            name: "osm.mapping.custom.host"
            value: "https://api.mapbox.com/styles/v1/mapbox/streets-v12/tiles/256/%z/%x/%y?access_token=" + mapboxApiKey
        }
        PluginParameter { name: "osm.useragent"; value: "Mozilla/5.0 (Qt6)" }
        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }
    }

    RouteQuery { id: routeQuery }

    RouteModel {
        id: routeModel
        plugin: mapPlugin
        query: routeQuery
        autoUpdate: false
        onStatusChanged: {
            if (status === RouteModel.Ready && count > 0) {
                var route = get(0)
                routeInfoUpdated((route.distance / 1000).toFixed(1) + " km", Math.round(route.travelTime / 60) + " min")
                if (route.segments && route.segments.length > 0 && route.segments[0].maneuver) {
                    nextInstruction = route.segments[0].maneuver.instructionText
                }
            }
        }
    }

    // Bandeau d'instructions
    Rectangle {
        id: instructionBanner
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 12 }
        width: parent.width * 0.8; height: 52; radius: 12
        color: "#dd171a21"; border.color: "#33ffffff"; border.width: 1
        visible: routeModel.status === RouteModel.Ready && nextInstruction.length > 0
        z: 10
        Text {
            anchors.fill: parent; anchors.margins: 12
            text: nextInstruction; color: "white"; font { pixelSize: 18; bold: true }
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap; elide: Text.ElideRight
        }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(carLat, carLon)

        // Calcul du zoom (vitesse ou manuel)
        zoomLevel: autoFollow ? Math.max(12, 18 - (carSpeed / 50.0)) : carZoom

        copyrightsVisible: false
        bearing: autoFollow ? carHeading : 0
        tilt: autoFollow ? 45 : 0

        // --- ANIMATIONS FLUIDES (LE COEUR DU SYSTEME) ---

        // 1. Animation de la position (Recentrage et suivi GPS lisse)
        Behavior on center {
            id: centerBehavior
            enabled: !mapDragHandler.active // On coupe si l'utilisateur glisse la carte
            CoordinateAnimation { duration: 800; easing.type: Easing.InOutQuad }
        }

        // 2. Animation du Zoom (Boutons et zoom auto)
        Behavior on zoomLevel {
            NumberAnimation { duration: 500; easing.type: Easing.InOutQuad }
        }

        // 3. Animation de la Rotation et Inclinaison
        Behavior on bearing { NumberAnimation { duration: 600 } }
        Behavior on tilt { NumberAnimation { duration: 600 } }

        // --- GESTION DES INTERACTIONS ---

        DragHandler {
            id: mapDragHandler
            onActiveChanged: if (active) root.autoFollow = false
        }

        WheelHandler {
            onWheel: (event) => {
                root.autoFollow = false
                root.carZoom = map.zoomLevel
            }
        }

        PinchHandler {
            onActiveChanged: if (active) root.autoFollow = false
        }

        // Itinéraire et Voiture (Inchangés)
        MapItemView {
            model: routeModel
            delegate: MapRoute { route: routeData; line.color: "#1db7ff"; line.width: 10; opacity: 0.8; smooth: true }
        }

        MapQuickItem {
            id: carMarker
            coordinate: QtPositioning.coordinate(carLat, carLon)
            anchorPoint.x: carVisual.width / 2; anchorPoint.y: carVisual.height / 2
            sourceItem: Item {
                id: carVisual
                width: 64; height: 64
                Rectangle { anchors.centerIn: parent; width: 50; height: 50; color: "#00a8ff"; opacity: 0.2; radius: 25 }
                Rectangle {
                    anchors.centerIn: parent; width: 24; height: 32; color: "white"; radius: 6; border.width: 2; border.color: "#171a21"
                    Rectangle { width: 24; height: 24; color: "white"; rotation: 45; y: -10; anchors.horizontalCenter: parent.horizontalCenter; border.width: 2; border.color: "#171a21" }
                    Rectangle { width: 26; height: 16; y: 20; x: -1; color: "white" }
                }
            }
        }
    }

    // --- LOGIQUE (Inchangée) ---
    onCarZoomChanged: { if (map.zoomLevel !== carZoom) autoFollow = false }

    function searchDestination(address) {
        var http = new XMLHttpRequest()
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(address) + ".json?access_token=" + mapboxApiKey + "&limit=1"
        http.open("GET", url, true);
        http.onreadystatechange = function() {
            if (http.readyState == 4 && http.status == 200) {
                try {
                    var json = JSON.parse(http.responseText)
                    if (json.features && json.features.length > 0) {
                        var c = json.features[0].center
                        routeQuery.clearWaypoints()
                        routeQuery.addWaypoint(QtPositioning.coordinate(carLat, carLon))
                        routeQuery.addWaypoint(QtPositioning.coordinate(c[1], c[0]))
                        routeModel.update()
                        autoFollow = true
                    }
                } catch(e) {}
            }
        }
        http.send();
    }

    function requestSuggestions(query) {
        if (!query || query.length < 3) return
        var http = new XMLHttpRequest()
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(query) + ".json?access_token=" + mapboxApiKey + "&autocomplete=true&limit=5"
        http.open("GET", url, true);
        http.onreadystatechange = function() {
            if (http.readyState == 4 && http.status == 200) {
                try {
                    var json = JSON.parse(http.responseText)
                    var results = []
                    if (json.features) for (var i=0; i<json.features.length; i++) results.push(json.features[i].place_name)
                    root.suggestionsUpdated(JSON.stringify(results))
                } catch(e) {}
            }
        }
        http.send();
    }

    onCarLatChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }
    onCarLonChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }

    function recenterMap() {
        autoFollow = true
        map.center = QtPositioning.coordinate(carLat, carLon)
        carZoom = 17
    }
}
