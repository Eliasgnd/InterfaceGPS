import QtQuick
import QtLocation
import QtPositioning
import QtQuick.Effects

Item {
    id: root
    width: 600; height: 400

    // --- PROPRIÉTÉS ---
    property double carLat: 48.2715
    property double carLon: 4.0645
    property double carZoom: 17
    property double carHeading: 0
    property double carSpeed: 0
    property bool autoFollow: true

    // NAVIGATION
    property string nextInstruction: ""
    property string distanceToNextTurn: ""
    property int nextManeuverDirection: 0

    property var currentRoute: null
    property int currentSegmentIndex: 0

    property int speedLimit: -1
    property double manualBearing: 0

    // Optimisation API
    property double lastApiCallTime: 0
    property var lastApiCallPos: QtPositioning.coordinate(0, 0)

    signal routeInfoUpdated(string distance, string duration)
    signal suggestionsUpdated(string suggestions)

    Plugin {
        id: mapPlugin
        name: "osm"
        PluginParameter {
            name: "osm.mapping.custom.host"
            value: "https://api.mapbox.com/styles/v1/mapbox/dark-v11/tiles/256/%z/%x/%y?access_token=" + mapboxApiKey
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
                currentRoute = get(0)
                currentSegmentIndex = 0
                routeInfoUpdated((currentRoute.distance / 1000).toFixed(1) + " km", Math.round(currentRoute.travelTime / 60) + " min")
                updateGuidance()
            }
        }
    }

    // --- TRADUCTION INTELLIGENTE ---
    function translateInstruction(enText, direction) {
        if (!enText) return ""

        console.log("DEBUG API - Texte reçu : " + enText + " | Direction : " + direction)

        var baseAction = ""

        // --- 1. Gestion avancée des Ronds-points ---
        var isRoundabout = enText.toLowerCase().indexOf("roundabout") !== -1 ||
                           enText.toLowerCase().indexOf("rotary") !== -1 ||
                           enText.toLowerCase().indexOf("circle") !== -1;

        if (isRoundabout) {
            // Tenter de trouver le numéro de sortie (chiffre ou lettre)
            var exitNum = "";

            // Cas 1 : "exit 2"
            var digitMatch = enText.match(/exit (\d+)/);
            if (digitMatch) exitNum = digitMatch[1];

            // Cas 2 : "second exit" (Mapbox écrit souvent en lettres)
            if (enText.indexOf("first") !== -1) exitNum = "1";
            else if (enText.indexOf("second") !== -1) exitNum = "2";
            else if (enText.indexOf("third") !== -1) exitNum = "3";
            else if (enText.indexOf("fourth") !== -1) exitNum = "4";
            else if (enText.indexOf("fifth") !== -1) exitNum = "5";

            if (exitNum !== "") {
                return "Rond-point : prenez la " + exitNum + (exitNum == "1" ? "ère" : "ème") + " sortie";
            }
            return "Prenez le rond-point";
        }

        // --- 2. Autres directions ---
        switch(direction) {
            case 1: baseAction = "Continuez tout droit"; break;
            case 2: baseAction = "Tenez la droite"; break;
            case 3: baseAction = "Légèrement à droite"; break;
            case 4: baseAction = "Tournez à droite"; break;
            case 5: baseAction = "Tournez fortement à droite"; break;
            case 6: baseAction = "Faites demi-tour"; break;
            case 7: baseAction = "Faites demi-tour"; break;
            case 8: baseAction = "Tournez fortement à gauche"; break;
            case 9: baseAction = "Tournez à gauche"; break;
            case 10: baseAction = "Légèrement à gauche"; break;
            case 11: baseAction = "Tenez la gauche"; break;
            default: baseAction = "Suivez la route";
        }

        var split = enText.split(" onto ")
        if (split.length > 1) return baseAction + " sur " + split[1]

        if (enText.indexOf("Arrive") !== -1 || enText.indexOf("Destination") !== -1) return "Vous êtes arrivé"

        return baseAction
    }

    // --- GUIDAGE DYNAMIQUE ---
    function updateGuidance() {
        if (!currentRoute || !currentRoute.segments) return
        var nextIndex = currentSegmentIndex + 1

        if (nextIndex >= currentRoute.segments.length) {
            nextInstruction = "Vous êtes arrivé"
            distanceToNextTurn = "0 m"
            nextManeuverDirection = 0
            return
        }

        var nextSegment = currentRoute.segments[nextIndex]
        var currentPos = QtPositioning.coordinate(carLat, carLon)
        var maneuverCoord = nextSegment.maneuver.position
        var dist = currentPos.distanceTo(maneuverCoord)

        // Lissage Waze
        if (dist >= 1000) distanceToNextTurn = (dist / 1000).toFixed(1) + " km"
        else if (dist >= 500) distanceToNextTurn = (Math.round(dist / 100) * 100) + " m"
        else if (dist >= 200) distanceToNextTurn = (Math.round(dist / 50) * 50) + " m"
        else if (dist >= 50) distanceToNextTurn = (Math.round(dist / 10) * 10) + " m"
        else distanceToNextTurn = Math.round(dist) + " m"

        // Traduction
        nextInstruction = translateInstruction(nextSegment.maneuver.instructionText, nextSegment.maneuver.direction)

        // --- CORRECTION BUG ROND-POINT ---
        // On vérifie "ond-point" pour éviter les erreurs de Majuscule/minuscule
        if (nextInstruction.toLowerCase().indexOf("ond-point") !== -1) {
            nextManeuverDirection = 100 // Force l'icône rond-point
        } else {
            nextManeuverDirection = nextSegment.maneuver.direction
        }

        if (dist < 30) {
            currentSegmentIndex++
            updateGuidance()
        }
    }

    // --- GESTION DES ICÔNES ---
    function getDirectionIconPath(direction) {
        var path = "qrc:/icons/" // Préfixe défini dans resources.qrc

        if (direction === 0) return path + "dir_arrival.svg"

        switch(direction) {
            case 1: return path + "dir_straight.svg"
            case 2: case 3: return path + "dir_slight_right.svg"
            case 4: case 5: return path + "dir_right.svg"
            case 6: case 7: return path + "dir_uturn.svg"
            case 8: case 9: return path + "dir_left.svg"
            case 10: case 11: return path + "dir_slight_left.svg"

            case 100: return path + "rond_point.svg" // Code spécial

            default: return path + "dir_straight.svg"
        }
    }

    function parseMaxspeedValue(raw) {
        if (!raw) return 0
        raw = ("" + raw).trim()
        if (raw.indexOf("FR:urban") === 0) return 50
        if (raw.indexOf("FR:rural") === 0) return 80
        if (raw.indexOf("FR:motorway") === 0) return 130
        var m = raw.match(/(\d+(\.\d+)?)/)
        if (!m) return 0
        var v = parseFloat(m[1])
        if (isNaN(v)) return 0
        if (raw.toLowerCase().indexOf("mph") !== -1) v = v * 1.60934
        return Math.round(v)
    }

    function updateRealSpeedLimit(lat, lon) {
        var http = new XMLHttpRequest()
        var radius = 80
        var query = '[out:json][timeout:25];(way(around:' + radius + ',' + lat + ',' + lon + ')["highway"]["maxspeed"];);out tags center;'
        var url = "https://overpass-api.de/api/interpreter"

        http.open("POST", url, true)
        http.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8")
        http.onreadystatechange = function() {
            if (http.readyState !== 4) return
            if (http.status !== 200) return
            try {
                var json = JSON.parse(http.responseText)
                if (!json.elements) return
                var bestLimit = 0
                var bestDist = 1e18
                var carPos = QtPositioning.coordinate(lat, lon)
                for (var i = 0; i < json.elements.length; i++) {
                    var el = json.elements[i]
                    if (!el.tags || !el.tags.maxspeed || !el.center) continue
                    var limit = parseMaxspeedValue(el.tags.maxspeed)
                    if (limit <= 0) continue
                    var p = QtPositioning.coordinate(el.center.lat, el.center.lon)
                    var d = carPos.distanceTo(p)
                    if (d < bestDist) {
                        bestDist = d
                        bestLimit = limit
                    }
                }
                if (bestLimit > 0) root.speedLimit = bestLimit
            } catch(e) {}
        }
        http.send("data=" + encodeURIComponent(query))
    }

    // --- CARTE ---
    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(carLat, carLon)
        zoomLevel: carZoom
        copyrightsVisible: false
        bearing: autoFollow ? carHeading : manualBearing
        tilt: autoFollow ? 45 : 0

        Behavior on center {
            enabled: !root.autoFollow && !mapDragHandler.active
            CoordinateAnimation { duration: 250; easing.type: Easing.InOutQuad }
        }
        Behavior on zoomLevel { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
        Behavior on bearing { NumberAnimation { duration: 600 } }
        Behavior on tilt { NumberAnimation { duration: 600 } }

        DragHandler { id: mapDragHandler; target: null; onCentroidChanged: { if (active && root.autoFollow) root.autoFollow = false; map.pan(-(centroid.position.x - mapDragHandler.centroid.pressedPosition.x), -(centroid.position.y - mapDragHandler.centroid.pressedPosition.y)) } }
        WheelHandler { onWheel: (event) => { var step = event.angleDelta.y > 0 ? 1 : -1; root.carZoom = Math.max(2, Math.min(20, root.carZoom + step)) } }

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
                enabled: false
                width: 64; height: 64
                Rectangle { anchors.centerIn: parent; width: 50; height: 50; color: "#00a8ff"; opacity: 0.2; radius: 25 }
                Rectangle {
                    anchors.centerIn: parent; width: 24; height: 32; color: "white"; radius: 6; border.width: 2; border.color: "#171a21"
                    Rectangle { width: 24; height: 24; color: "white"; rotation: 45; y: -10; anchors.horizontalCenter: parent.horizontalCenter; border.width: 2; border.color: "#171a21" }
                }
            }
        }
    }

    // --- BANDEAU NAVIGATION ---
    Rectangle {
        id: navPanel
        visible: routeModel.status === RouteModel.Ready && nextInstruction.length > 0

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 15

        width: Math.min(parent.width * 0.9, 400)
        height: 70
        radius: 35

        color: "#CC1C1C1E"
        border.color: "#33FFFFFF"
        border.width: 1

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#80000000"
            shadowBlur: 10
            shadowVerticalOffset: 4
        }

        Row {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 15

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 40; height: 40
                source: getDirectionIconPath(nextManeuverDirection)
                sourceSize.width: 40
                sourceSize.height: 40
                fillMode: Image.PreserveAspectFit
                mipmap: true
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 65
                spacing: 2

                Text {
                    text: distanceToNextTurn
                    color: "white"
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    text: nextInstruction
                    color: "#D0D0D0"
                    font.pixelSize: 14
                    elide: Text.ElideRight
                    width: parent.width
                    maximumLineCount: 1
                }
            }
        }
    }

    Rectangle {
        id: speedSign
        width: 60; height: 60; radius: 30
        color: "white"; border.color: "red"; border.width: 6
        anchors { bottom: parent.bottom; left: parent.left; margins: 20 }
        z: 20
        visible: speedLimit > 0

        Rectangle {
            anchors.fill: parent; radius: 30
            color: "red"; visible: speedLimit > 0 && carSpeed > speedLimit; opacity: 0.6
            SequentialAnimation on opacity {
                running: speedLimit > 0 && carSpeed > speedLimit
                loops: Animation.Infinite
                NumberAnimation { from: 0.2; to: 0.8; duration: 500 }
                NumberAnimation { from: 0.8; to: 0.2; duration: 500 }
            }
        }
        Text {
            anchors.centerIn: parent
            text: speedLimit < 0 ? "-" : speedLimit; font { pixelSize: 24; bold: true }
            color: "black"
        }
    }

    onCarLatChanged: {
        if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon)
        updateGuidance()
        var now = Date.now()
        var currentPos = QtPositioning.coordinate(carLat, carLon)
        var distanceSinceLast = lastApiCallPos.distanceTo(currentPos)
        if ( (now - lastApiCallTime > 15000) && (distanceSinceLast > 50) ) {
            updateRealSpeedLimit(carLat, carLon)
            lastApiCallTime = now
            lastApiCallPos = currentPos
        }
    }
    onCarLonChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }
    onCarZoomChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }

    function searchDestination(address) {
        var http = new XMLHttpRequest()
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(address) + ".json?access_token=" + mapboxApiKey + "&limit=1&language=fr"
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
                        updateRealSpeedLimit(carLat, carLon)
                    }
                } catch(e) {}
            }
        }
        http.send();
    }

    function requestSuggestions(query) {
        if (!query || query.length < 3) return
        var http = new XMLHttpRequest()
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(query) + ".json?access_token=" + mapboxApiKey + "&autocomplete=true&limit=5&language=fr"
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

    function recenterMap() {
        autoFollow = true
        map.center = QtPositioning.coordinate(carLat, carLon)
        carZoom = 17
    }
}
