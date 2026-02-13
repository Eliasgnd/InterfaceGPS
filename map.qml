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

    // GUIDAGE AVANCÉ
    property var routeSteps: []
    property int currentStepIndex: 0
    property double lastDistToStep: 999999
    signal routeReadyForSimulation(var pathObj)

    // STATISTIQUES
    property string remainingDistString: "-- km"
    property string remainingTimeString: "-- min"
    property string arrivalTimeString: "--:--"
    property real realRouteSpeed: 13.8

    // TRACÉ
    property var routePoints: []
    property var routeSpeedLimits: []
    property var finalDestination: null
    property bool isRecalculating: false

    property int speedLimit: -1
    property double manualBearing: 0
    property double lastApiCallTime: 0
    property var lastApiCallPos: QtPositioning.coordinate(0, 0)

    signal routeInfoUpdated(string distance, string duration)
    signal suggestionsUpdated(string suggestions)

    // --- PLUGIN CARTE (MAPBOX via OSM) ---
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

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(carLat, carLon)
        zoomLevel: carZoom
        copyrightsVisible: false
        bearing: autoFollow ? carHeading : manualBearing
        tilt: autoFollow ? 45 : 0

        Behavior on center { enabled: !root.autoFollow && !mapDragHandler.active; CoordinateAnimation { duration: 250; easing.type: Easing.InOutQuad } }
        Behavior on zoomLevel { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
        Behavior on bearing {
            RotationAnimation {
                direction: RotationAnimation.Shortest
                duration: 600
            }
        }
        Behavior on tilt { NumberAnimation { duration: 600 } }

        DragHandler {
            id: mapDragHandler
            target: null
            onCentroidChanged: {
                if (!active) return
                if (root.autoFollow) root.autoFollow = false
                if (centroid.pressedPosition) map.pan(-(centroid.position.x - centroid.pressedPosition.x), -(centroid.position.y - centroid.pressedPosition.y))
            }
        }
        WheelHandler { onWheel: (event) => { var step = event.angleDelta.y > 0 ? 1 : -1; root.carZoom = Math.max(2, Math.min(20, root.carZoom + step)) } }

        MapPolyline {
            id: visualRouteLine
            line.width: 10
            line.color: "#1db7ff"
            opacity: 0.8
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

    // --- CALCUL D'ITINÉRAIRE (API Mapbox Traffic) ---
    function requestRouteWithTraffic(startCoord, endCoord) {
            var url = "https://api.mapbox.com/directions/v5/mapbox/driving-traffic/" +
                      startCoord.longitude + "," + startCoord.latitude + ";" +
                      endCoord.longitude + "," + endCoord.latitude +
                      // ON RAJOUTE &annotations=maxspeed ICI :
                      "?geometries=geojson&steps=true&overview=full&language=fr&annotations=maxspeed&access_token=" + mapboxApiKey;

        var http = new XMLHttpRequest()
        http.open("GET", url, true);
        http.onreadystatechange = function() {
            if (http.readyState == 4) {
                if (http.status == 200) {
                    try {
                        var json = JSON.parse(http.responseText);
                        if (json.routes && json.routes.length > 0) {
                            var route = json.routes[0];

                            var durationSec = route.duration;
                            var distMeters = route.distance;

                            if (durationSec > 0) realRouteSpeed = distMeters / durationSec;
                            else realRouteSpeed = 13.8;

                            routeInfoUpdated((distMeters / 1000).toFixed(1) + " km", Math.round(durationSec / 60) + " min");
                            updateStatsFromDuration(durationSec, distMeters);

                            var coords = route.geometry.coordinates;
                            var newPoints = [];
                            for (var i = 0; i < coords.length; i++) {
                                newPoints.push(QtPositioning.coordinate(coords[i][1], coords[i][0]));
                            }
                            var simplePath = [];
                            for (var j = 0; j < coords.length; j++) {
                                simplePath.push({"lat": coords[j][1], "lon": coords[j][0]});
                            }
                            routeReadyForSimulation(simplePath);
                            routePoints = newPoints;
                            visualRouteLine.path = routePoints;

                            if (route.legs && route.legs[0] && route.legs[0].annotation && route.legs[0].annotation.maxspeed) {
                                                            root.routeSpeedLimits = route.legs[0].annotation.maxspeed;
                                                        } else {
                                                            root.routeSpeedLimits = [];
                                                        }

                            if (route.legs && route.legs.length > 0) {
                                routeSteps = route.legs[0].steps;
                                currentStepIndex = 0;
                                lastDistToStep = 999999;
                                updateGuidance();
                            }
                            isRecalculating = false;
                        }
                    } catch(e) { console.log("Erreur JSON: " + e) }
                }
            }
        }
        http.send();
    }

    // --- LOGIQUE WAZE (ARRONDIS ET PASSAGE DE VIRAGE) ---
    function formatWazeDistance(meters) {
        if (meters < 20) return "0 m";
        if (meters < 300) return (Math.round(meters / 10) * 10) + " m";
        if (meters < 1000) return (Math.round(meters / 50) * 50) + " m";
        return (Math.round(meters / 100) / 10).toFixed(1) + " km";
    }

    function updateGuidance() {
        if (!routeSteps || routeSteps.length === 0 || currentStepIndex >= routeSteps.length) {
            if (routePoints.length > 0 && routePoints.length < 15) {
                 nextInstruction = "Vous êtes arrivé";
                 distanceToNextTurn = "0 m";
                 nextManeuverDirection = 0;
            }
            return;
        }

        var step = routeSteps[currentStepIndex];
        var maneuverLoc = QtPositioning.coordinate(step.maneuver.location[1], step.maneuver.location[0]);
        var carPos = QtPositioning.coordinate(carLat, carLon);
        var dist = carPos.distanceTo(maneuverLoc);

        if (dist < 35 && dist > lastDistToStep && currentStepIndex < routeSteps.length - 1) {
            currentStepIndex++;
            lastDistToStep = 999999;
            updateGuidance();
            return;
        }
        lastDistToStep = dist;

        distanceToNextTurn = formatWazeDistance(dist);
        nextInstruction = step.maneuver.instruction;
        nextManeuverDirection = mapboxModifierToDirection(step.maneuver.type, step.maneuver.modifier);
    }

    function mapboxModifierToDirection(type, modifier) {
        if (type === "arrive") return 0;
        if (type === "roundabout" || type === "rotary") return 100;
        if (!modifier) return 1;
        if (modifier.indexOf("slight right") !== -1) return 3;
        if (modifier.indexOf("sharp right") !== -1) return 5;
        if (modifier.indexOf("right") !== -1) return 4;
        if (modifier.indexOf("slight left") !== -1) return 10;
        if (modifier.indexOf("sharp left") !== -1) return 8;
        if (modifier.indexOf("left") !== -1) return 9;
        if (modifier.indexOf("uturn") !== -1) return 6;
        return 1;
    }

    function updateTripStats() {
        if (routePoints.length === 0) return;
        var carPos = QtPositioning.coordinate(carLat, carLon);
        var distRemaining = 0;
        distRemaining += carPos.distanceTo(routePoints[0]);
        for (var i = 0; i < routePoints.length - 1; i++) {
            distRemaining += routePoints[i].distanceTo(routePoints[i+1]);
        }
        remainingDistString = formatWazeDistance(distRemaining);
        var timeSeconds = distRemaining / realRouteSpeed;
        updateStatsFromDuration(timeSeconds, distRemaining);
    }

    function updateStatsFromDuration(durationSec, distMeters) {
        var h = Math.floor(durationSec / 3600);
        var m = Math.floor((durationSec % 3600) / 60);
        if (h > 0) remainingTimeString = h + " h " + (m < 10 ? "0"+m : m);
        else remainingTimeString = m + " min";
        var arrivalDate = new Date(Date.now() + durationSec * 1000);
        var ah = arrivalDate.getHours();
        var am = arrivalDate.getMinutes();
        arrivalTimeString = ah + ":" + (am < 10 ? "0"+am : am);
    }

    // --- CORRECTION DÉFINITIVE : Mise à jour du tracé visuel ---
        function updateRouteVisuals() {
            if (!root.routePoints || root.routePoints.length === 0) {
                visualRouteLine.path = [];
                root.speedLimit = -1;
                return;
            }

            var carPos = QtPositioning.coordinate(root.carLat, root.carLon);
            var changed = false;
            var tempPoints = root.routePoints;
            var tempLimits = (typeof root.routeSpeedLimits !== "undefined" && root.routeSpeedLimits) ? root.routeSpeedLimits : [];

            // Suppression des points qu'on a physiquement dépassés
            if (tempPoints.length > 1 && carPos.distanceTo(tempPoints[1]) < 35) {
                tempPoints.splice(0, 1);
                if (tempLimits.length > 0) tempLimits.splice(0, 1);
                changed = true;
            } else if (tempPoints.length > 2 && carPos.distanceTo(tempPoints[2]) < 35) {
                tempPoints.splice(0, 2);
                if (tempLimits.length > 1) tempLimits.splice(0, 2);
                changed = true;
            }

            if (changed) {
                root.routePoints = tempPoints;
                if (typeof root.routeSpeedLimits !== "undefined") {
                    root.routeSpeedLimits = tempLimits;
                }
            }

            if (tempLimits.length > 0) {
                var limitData = tempLimits[0];
                if (limitData && limitData.speed !== undefined) {
                     root.speedLimit = limitData.speed;
                } else if (typeof limitData === 'number') {
                     root.speedLimit = limitData;
                }
            }

            // --- NOUVEAU : DESSIN INTELLIGENT DE LA LIGNE ---
            if (tempPoints.length > 0) {
                var drawPath = [carPos];
                var limit = Math.min(tempPoints.length, 3000);

                // On calcule l'angle (en degrés) entre la position de la voiture et le prochain point GPS
                var bearingTo0 = carPos.azimuthTo(tempPoints[0]);

                // On le compare à la direction (le cap) vers laquelle la voiture roule actuellement
                var diff = Math.abs(bearingTo0 - root.carHeading);
                if (diff > 180) diff = 360 - diff;

                // Si la différence d'angle est supérieure à 90°, c'est que le point est DERRIÈRE nous.
                // On l'ignore (startIndex = 1) pour ne pas faire un trait en arrière.
                // Sinon, le point est DEVANT nous (ex: un virage !), on le trace (startIndex = 0).
                var startIndex = (diff > 90) ? 1 : 0;

                for (var k = startIndex; k < limit; k++) {
                    drawPath.push(tempPoints[k]);
                }

                visualRouteLine.path = drawPath;
            } else {
                visualRouteLine.path = [];
            }
        }

        // --- FONCTIONS MATHÉMATIQUES ET LOGS DE DEBUG ---
        function distanceToSegment(P, A, B) {
            var d_AB = A.distanceTo(B);
            if (d_AB < 1) return A.distanceTo(P);

            var d_AP = A.distanceTo(P);
            var bearingAB = A.azimuthTo(B);
            var bearingAP = A.azimuthTo(P);

            var angleDiff = (bearingAP - bearingAB) * Math.PI / 180.0;
            var crossTrack = Math.abs(d_AP * Math.sin(angleDiff));
            var alongTrack = d_AP * Math.cos(angleDiff);

            if (alongTrack < 0) return d_AP;
            if (alongTrack > d_AB) return P.distanceTo(B);

            return crossTrack;
        }

        function checkIfOffRoute() {
            if (routePoints.length < 2 || isRecalculating) return;

            var carPos = QtPositioning.coordinate(carLat, carLon);
            var minDistance = 100000;
            var bestIndex = -1;

            var searchLimit = Math.min(routePoints.length - 1, 30);

            for (var i = 0; i < searchLimit; i++) {
                var d = distanceToSegment(carPos, routePoints[i], routePoints[i+1]);
                if (d < minDistance) {
                    minDistance = d;
                    bestIndex = i; // On mémorise le meilleur segment
                }
            }

            // === LOGS DE DEBUG ===
            // On affiche les logs seulement si la distance commence à être suspecte (> 25 mètres)
            // pour ne pas saturer la console quand on roule bien.
            if (minDistance > 25) {
                console.log("🔍 [DEBUG GPS] Analyse de sortie de route...");
                console.log("   📍 Voiture : " + carPos.latitude.toFixed(5) + ", " + carPos.longitude.toFixed(5));
                console.log("   📏 Distance au segment (Index " + bestIndex + ") : " + Math.round(minDistance) + " mètres");
                if (bestIndex >= 0) {
                    var pA = routePoints[bestIndex];
                    var pB = routePoints[bestIndex+1];
                    console.log("   🔗 Segment ciblé : A[" + pA.latitude.toFixed(5) + "," + pA.longitude.toFixed(5) + "] -> B[" + pB.latitude.toFixed(5) + "," + pB.longitude.toFixed(5) + "]");
                    console.log("   🚗 Distance de la voiture à A : " + Math.round(carPos.distanceTo(pA)) + "m | à B : " + Math.round(carPos.distanceTo(pB)) + "m");
                }
                console.log("-----------------------");
            }

            // 75 mètres de tolérance avant le recalcul
            if (minDistance > 75) {
                console.log("⚠️ RECALCUL DÉCLENCHÉ ! La distance de " + Math.round(minDistance) + "m dépasse 75m.");
                recalculateRoute();
            }
        }

    // --- RECHERCHE ET SUGGESTIONS (CORRIGÉES) ---
    function searchDestination(address) {
        var queryUrl = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(address) + ".json?access_token=" + mapboxApiKey + "&limit=1&language=fr";
        var http = new XMLHttpRequest();
        http.open("GET", queryUrl, true);
        http.onreadystatechange = function() {
            if (http.readyState == 4 && http.status == 200) {
                try {
                    var json = JSON.parse(http.responseText);
                    if (json.features && json.features.length > 0) {
                        var c = json.features[0].center;
                        finalDestination = QtPositioning.coordinate(c[1], c[0]);
                        isRecalculating = true;
                        nextInstruction = "Calcul...";
                        requestRouteWithTraffic(QtPositioning.coordinate(carLat, carLon), finalDestination);
                        autoFollow = true;
                        updateRealSpeedLimit(carLat, carLon);
                    }
                } catch(e) {}
            }
        }
        http.send();
    }

    function requestSuggestions(query) {
        if (!query || query.length < 3) return;
        var http = new XMLHttpRequest();
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(query) + ".json?access_token=" + mapboxApiKey + "&autocomplete=true&limit=5&language=fr";

        // CORRECTION ICI : On ajoute le .open() manquant
        http.open("GET", url, true);

        http.onreadystatechange = function() {
            if (http.readyState == 4 && http.status == 200) {
                try {
                    var json = JSON.parse(http.responseText);
                    var results = [];
                    if (json.features) for (var i=0; i<json.features.length; i++) results.push(json.features[i].place_name);
                    root.suggestionsUpdated(JSON.stringify(results));
                } catch(e) {}
            }
        }
        http.send();
    }

    function recalculateRoute() {
        if (!finalDestination || isRecalculating) return;
        isRecalculating = true;
        requestRouteWithTraffic(QtPositioning.coordinate(carLat, carLon), finalDestination);
    }

    function recenterMap() {
        autoFollow = true;
        map.center = QtPositioning.coordinate(carLat, carLon);
        carZoom = 17;
    }

    // --- LIMITES DE VITESSE ---
    function updateRealSpeedLimit(lat, lon) {
        var http = new XMLHttpRequest();
        var query = '[out:json][timeout:25];(way(around:80,' + lat + ',' + lon + ')["highway"]["maxspeed"];);out tags center;';
        http.open("POST", "https://overpass-api.de/api/interpreter", true);
        http.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
        http.onreadystatechange = function() {
            if (http.readyState === 4 && http.status === 200) {
                try {
                    var json = JSON.parse(http.responseText);
                    if (json.elements) {
                        var bestLimit = 0; var bestDist = 1e18;
                        var carPos = QtPositioning.coordinate(lat, lon);
                        for (var i = 0; i < json.elements.length; i++) {
                            var el = json.elements[i];
                            if (el.tags && el.tags.maxspeed) {
                                var limit = parseMaxspeedValue(el.tags.maxspeed);
                                if (limit > 0) {
                                    var p = QtPositioning.coordinate(el.center.lat, el.center.lon);
                                    var d = carPos.distanceTo(p);
                                    if (d < bestDist) { bestDist = d; bestLimit = limit; }
                                }
                            }
                        }
                        if (bestLimit > 0) root.speedLimit = bestLimit;
                    }
                } catch(e) {}
            }
        }
        http.send("data=" + encodeURIComponent(query));
    }

    function parseMaxspeedValue(raw) {
        if (!raw) return 0;
        raw = ("" + raw).trim();
        if (raw.indexOf("FR:urban") === 0) return 50;
        if (raw.indexOf("FR:rural") === 0) return 80;
        if (raw.indexOf("FR:motorway") === 0) return 130;
        var m = raw.match(/(\d+)/);
        if (m) return parseFloat(m[1]);
        return 0;
    }

    function getDirectionIconPath(direction) {
        var path = "qrc:/icons/";
        if (direction === 0) return path + "dir_arrival.svg";
        switch(direction) {
            case 1: return path + "dir_straight.svg";
            case 3: return path + "dir_slight_right.svg";
            case 4: return path + "dir_right.svg";
            case 5: return path + "dir_right.svg";
            case 6: return path + "dir_uturn.svg";
            case 8: return path + "dir_left.svg";
            case 9: return path + "dir_left.svg";
            case 10: return path + "dir_slight_left.svg";
            case 100: return path + "rond_point.svg";
            default: return path + "dir_straight.svg";
        }
    }

    // --- UI ELEMENTS ---
    Rectangle {
        id: navPanel
        visible: (routePoints.length > 0 || isRecalculating) && nextInstruction.length > 0
        anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: 15
        width: Math.min(parent.width * 0.9, 500); height: 70; radius: 35
        color: "#CC1C1C1E"; border.color: isRecalculating ? "#FF9800" : "#33FFFFFF"; border.width: 2
        layer.enabled: true
        layer.effect: MultiEffect { shadowEnabled: true; shadowColor: "#80000000"; shadowBlur: 10; shadowVerticalOffset: 4 }
        Row {
            anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20; spacing: 15
            Image {
                anchors.verticalCenter: parent.verticalCenter; width: 40; height: 40
                source: getDirectionIconPath(nextManeuverDirection); sourceSize.width: 40; sourceSize.height: 40; fillMode: Image.PreserveAspectFit; mipmap: true
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter; width: parent.width - 65; spacing: 2
                Text { text: distanceToNextTurn; color: isRecalculating ? "#FF9800" : "white"; font.pixelSize: 22; font.bold: true }
                Text { text: nextInstruction; color: "#D0D0D0"; font.pixelSize: 14; elide: Text.ElideRight; width: parent.width; maximumLineCount: 1 }
            }
        }
    }

    Rectangle {
        id: bottomInfoPanel
        visible: routePoints.length > 0 && !isRecalculating
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 20
        width: Math.min(parent.width * 0.5, 250); height: 50; radius: 25
        color: "#CC1C1C1E"; border.color: "#33FFFFFF"; border.width: 1
        Row {
            anchors.centerIn: parent; spacing: 15
            Text { text: remainingTimeString; color: "#4CAF50"; font.bold: true; font.pixelSize: 18 }
            Text { text: "•"; color: "#808080"; font.pixelSize: 18 }
            Text { text: remainingDistString; color: "#E0E0E0"; font.pixelSize: 18 }
            Text { text: "(" + arrivalTimeString + ")"; color: "#808080"; font.pixelSize: 18; anchors.verticalCenter: parent.verticalCenter }
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
            anchors.fill: parent; radius: 30; color: "red"; visible: speedLimit > 0 && carSpeed > speedLimit; opacity: 0.6
            SequentialAnimation on opacity {
                running: speedLimit > 0 && carSpeed > speedLimit; loops: Animation.Infinite
                NumberAnimation { from: 0.2; to: 0.8; duration: 500 }
                NumberAnimation { from: 0.8; to: 0.2; duration: 500 }
            }
        }
        Text {
            anchors.centerIn: parent
            text: speedLimit < 0 ? "-" : speedLimit
            font.pixelSize: 24
            font.bold: true
            color: "black"
        }
    }

    onCarLatChanged: {
            if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon);
            if (!isRecalculating) {
                updateRouteVisuals();
                checkIfOffRoute();
                updateTripStats();
                updateGuidance();
            }
        }
        onCarLonChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon); }
        onCarZoomChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon); }
}
