import QtQuick
import QtLocation
import QtPositioning
import QtQuick.Effects
import QtQuick.Shapes

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

    // Zoom Intelligent
    property bool enableSpeedZoom: true
    property bool internalZoomChange: false

    // NAVIGATION
    property string nextInstruction: ""
    property string distanceToNextTurn: ""
    property int nextManeuverDirection: 0

    // GUIDAGE AVANCÉ
    property var routeSteps: []
    property int currentStepIndex: 0
    property double lastDistToStep: 999999

    // STATISTIQUES
    property string remainingDistString: "-- km"
    property string remainingTimeString: "-- min"
    property string arrivalTimeString: "--:--"
    property real realRouteSpeed: 13.8

    // TRACÉ ET TRAFIC MULTICOLORE
    property var routePoints: []
    property var finalDestination: null
    property bool isRecalculating: false

    property int speedLimit: -1
    property double manualBearing: 0

    property var routeSpeedLimits: []
    property var routeCongestions: []
    property var trafficSegments: [] // Ne gère que l'orange et le rouge

    // SIGNAUX
    signal routeInfoUpdated(string distance, string duration)
    signal suggestionsUpdated(string suggestions)
    signal routeReadyForSimulation(var pathObj)

    // --- 1. PLUGIN STANDARD (OSM + CartoDB Dark) ---
    Plugin {
        id: mapPlugin
        name: "osm"

        PluginParameter {
            name: "osm.mapping.custom.host"
            value: "https://a.basemaps.cartocdn.com/dark_all/%z/%x/%y.png"
        }
        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }
        PluginParameter { name: "osm.useragent"; value: "GPSInterface/1.0" }
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

        // Animations fluides
        Behavior on center { enabled: !root.autoFollow && !mapDragHandler.active; CoordinateAnimation { duration: 250; easing.type: Easing.InOutQuad } }
        Behavior on zoomLevel { NumberAnimation { duration: 800; easing.type: Easing.InOutQuad } }
        Behavior on bearing { RotationAnimation { direction: RotationAnimation.Shortest; duration: 600 } }
        Behavior on tilt { NumberAnimation { duration: 800 } }

        // --- GESTION DU CLIC SUR LA CARTE ---
        MouseArea {
            anchors.fill: parent

            // Important pour laisser passer le drag (glissement) de la carte
            propagateComposedEvents: true
            preventStealing: false

            onClicked: (mouse) => {
                // 1. Conversion des pixels (x,y) en Latitude/Longitude
                var coord = map.toCoordinate(Qt.point(mouse.x, mouse.y))

                if (coord.isValid) {
                    console.log("Destination définie au clic : " + coord.latitude + ", " + coord.longitude)

                    // 2. Mise à jour de la variable de destination
                    root.finalDestination = coord

                    // 3. Feedback visuel
                    root.nextInstruction = "Calcul de l'itinéraire..."
                    root.isRecalculating = true

                    // 4. Lancement du calcul d'itinéraire existant
                    root.requestRouteWithTraffic(
                        QtPositioning.coordinate(root.carLat, root.carLon),
                        coord
                    )

                    // 5. Désactiver le suivi automatique pour ne pas recentrer immédiatement la vue
                    root.autoFollow = false
                }
            }
        }

        DragHandler {
            id: mapDragHandler
            target: null
            onCentroidChanged: {
                if (!active) return
                if (root.autoFollow) root.autoFollow = false
                if (centroid.pressedPosition) map.pan(-(centroid.position.x - centroid.pressedPosition.x), -(centroid.position.y - centroid.pressedPosition.y))
            }
        }

        WheelHandler {
            onWheel: (event) => {
                var step = event.angleDelta.y > 0 ? 1 : -1;
                root.carZoom = Math.max(2, Math.min(20, root.carZoom + step))
            }
        }

        // --- DESSINATEUR DE ROUTE ---
        // 1. La ligne bleue de base (mise à jour en temps réel)
        MapPolyline {
            id: visualRouteLine
            line.width: 8
            line.color: "#1db7ff"
            opacity: 0.9
            z: 1
        }

        // 2. Les lignes de trafic Orange/Rouge superposées (mises à jour occasionnellement pour zéro lag)
        MapItemView {
            model: root.trafficSegments
            delegate: MapPolyline {
                line.width: 8
                line.color: modelData.color
                path: modelData.path
                opacity: 1.0
                z: 2
            }
        }

        // 3. Marqueur de destination (Drapeau d'arrivée)
        MapQuickItem {
            visible: root.finalDestination !== null
            coordinate: root.finalDestination !== null ? root.finalDestination : QtPositioning.coordinate(0,0)
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height // La pointe en bas
            z: 5

            sourceItem: Image {
                width: 40
                height: 40
                source: "qrc:/icons/dir_arrival.svg"
                fillMode: Image.PreserveAspectFit
            }
        }

        MapQuickItem {
            id: carMarker
            coordinate: QtPositioning.coordinate(carLat, carLon)
            anchorPoint.x: carVisual.width / 2; anchorPoint.y: carVisual.height / 2
            z: 10

            sourceItem: Item {
                id: carVisual
                width: 100; height: 100
                enabled: false

                // --- LE HALO INTELLIGENT ---
                Rectangle {
                    id: haloRect
                    anchors.centerIn: parent
                    width: 70; height: 70; radius: 35
                    color: "#D2CAEC"; opacity: 0

                    // Condition : Route en cours ET (vitesse très faible OU arrivé)
                    property bool pulseActive: (root.routePoints && root.routePoints.length > 0) && (root.carSpeed < 2 || nextInstruction === "Vous êtes arrivé")

                    SequentialAnimation {
                        running: haloRect.pulseActive
                        loops: Animation.Infinite
                        alwaysRunToEnd: true

                        ParallelAnimation {
                            NumberAnimation { target: haloRect; property: "opacity"; from: 0.6; to: 0.0; duration: 1500; easing.type: Easing.OutQuad }
                            NumberAnimation { target: haloRect; property: "width"; from: 40; to: 90; duration: 1500; easing.type: Easing.OutQuad }
                            NumberAnimation { target: haloRect; property: "height"; from: 40; to: 90; duration: 1500; easing.type: Easing.OutQuad }
                        }
                    }
                }

                // --- LA FLÈCHE STYLE WAZE ---
                Item {
                    anchors.centerIn: parent
                    width: 60; height: 60

                    Shape {
                        anchors.fill: parent

                        ShapePath {
                            strokeWidth: 4          // Grosse bordure blanche Waze
                            strokeColor: "#370028"  // Bordure Blanche
                            fillColor: "#C9A0DC"    // Intérieur Cyan

                            // Bords et pointes arrondis !
                            joinStyle: ShapePath.RoundJoin
                            capStyle: ShapePath.RoundCap

                            // Proportions "trapues" fidèles à Waze
                            startX: 30; startY: 10    // Pointe haut
                            PathLine { x: 48; y: 46 } // Bas droite
                            PathLine { x: 30; y: 36 } // Creux central (moins profond)
                            PathLine { x: 12; y: 46 } // Bas gauche
                            PathLine { x: 30; y: 10 } // Retour
                        }
                    }

                    // L'ombre portée pour l'effet "Flottant 3D"
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: "#A0000000"
                        shadowBlur: 10
                        shadowVerticalOffset: 3
                    }
                }
            }
        }
    }

    // --- REQUÊTES API ---
    function requestRouteWithTraffic(startCoord, endCoord) {
        if (typeof mapboxApiKey === "undefined" || mapboxApiKey === "") return;

        var url = "https://api.mapbox.com/directions/v5/mapbox/driving-traffic/" +
                  startCoord.longitude + "," + startCoord.latitude + ";" +
                  endCoord.longitude + "," + endCoord.latitude +
                  "?geometries=geojson&steps=true&overview=full&language=fr&annotations=maxspeed,congestion&access_token=" + mapboxApiKey;

        var http = new XMLHttpRequest()
        http.open("GET", url, true);
        http.onreadystatechange = function() {
            if (http.readyState == 4 && http.status == 200) {
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

                        // Extraction des points
                        var coords = route.geometry.coordinates;
                        var newPoints = [];
                        var simplePath = [];
                        for (var i = 0; i < coords.length; i++) {
                            newPoints.push(QtPositioning.coordinate(coords[i][1], coords[i][0]));
                            simplePath.push({"lat": coords[i][1], "lon": coords[i][0]});
                        }
                        routePoints = newPoints;
                        root.trafficSegments = []; // On réinitialise le trafic

                        routeReadyForSimulation(simplePath);

                        // Vitesses et Trafic
                        if (route.legs && route.legs[0] && route.legs[0].annotation) {
                            routeSpeedLimits = route.legs[0].annotation.maxspeed ? route.legs[0].annotation.maxspeed : [];
                            routeCongestions = route.legs[0].annotation.congestion ? route.legs[0].annotation.congestion : [];
                        } else {
                            routeSpeedLimits = [];
                            routeCongestions = [];
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
        http.send();
    }

    // --- FONCTIONS VISUELLES ET TRAFIC ---
    function updateRouteVisuals() {
        if (!root.routePoints || root.routePoints.length === 0) {
            root.trafficSegments = [];
            visualRouteLine.path = [];
            root.speedLimit = -1;
            return;
        }

        var carPos = QtPositioning.coordinate(root.carLat, root.carLon);
        var changed = false;
        var tempPoints = root.routePoints;
        var tempLimits = (typeof root.routeSpeedLimits !== "undefined" && root.routeSpeedLimits) ? root.routeSpeedLimits : [];
        var tempCongestions = (typeof root.routeCongestions !== "undefined" && root.routeCongestions) ? root.routeCongestions : [];

        // ON TROUVE LE SEGMENT LE PLUS PROCHE POUR ÉVITER DE COUPER LES BÂTIMENTS
        var bestI = 0;
        var minD = 100000;
        var searchLimit = Math.min(tempPoints.length - 1, 30);
        for (var j = 0; j < searchLimit; j++) {
            var d = distanceToSegment(carPos, tempPoints[j], tempPoints[j+1]);
            if (d < minD) {
                minD = d;
                bestI = j;
            }
        }

        // On efface intelligemment tous les points qui sont derrière la voiture
        if (bestI > 0) {
            tempPoints.splice(0, bestI);
            if (tempLimits.length >= bestI) tempLimits.splice(0, bestI);
            if (tempCongestions.length >= bestI) tempCongestions.splice(0, bestI);
            changed = true;
        }

        if (changed) {
            root.routePoints = tempPoints;
            if (typeof root.routeSpeedLimits !== "undefined") root.routeSpeedLimits = tempLimits;
            if (typeof root.routeCongestions !== "undefined") root.routeCongestions = tempCongestions;
        }

        if (tempLimits.length > 0) {
            var limitData = tempLimits[0];
            if (limitData && limitData.speed !== undefined) root.speedLimit = limitData.speed;
            else if (typeof limitData === 'number') root.speedLimit = limitData;
        }

        // 1. DESSIN DE LA LIGNE BLEUE (Mise à jour ultra rapide, suit parfaitement la route)
        var limit = Math.min(tempPoints.length, 3000);
        var drawPath = [carPos];
        if (tempPoints.length > 1) {
            for (var k = 1; k < limit; k++) drawPath.push(tempPoints[k]);
        } else if (tempPoints.length === 1) {
            drawPath.push(tempPoints[0]);
        }
        visualRouteLine.path = drawPath;

        // 2. DESSIN DU TRAFIC ORANGE/ROUGE (Mis à jour que quand c'est nécessaire -> Anti-Lag)
        if (changed || typeof root.trafficSegments === "undefined" || root.trafficSegments.length === 0) {
            var newSegments = [];
            var currentPath = [];
            var currentColor = "none";

            for (var k = 0; k < limit - 1; k++) {
                var levelK = (k < tempCongestions.length) ? tempCongestions[k] : "unknown";
                var colorK = "none"; // Par défaut, on ne dessine rien (on laisse voir le bleu)

                if (levelK === "moderate") colorK = "#FF9800"; // Orange
                else if (levelK === "heavy" || levelK === "severe") colorK = "#F44336"; // Rouge

                if (colorK === "none") {
                    if (currentColor !== "none") {
                        newSegments.push({"path": currentPath, "color": currentColor});
                        currentColor = "none";
                        currentPath = [];
                    }
                } else {
                    if (currentColor === colorK) {
                        currentPath.push(tempPoints[k+1]);
                    } else {
                        if (currentColor !== "none") {
                            newSegments.push({"path": currentPath, "color": currentColor});
                        }
                        currentPath = [tempPoints[k], tempPoints[k+1]];
                        currentColor = colorK;
                    }
                }
            }
            if (currentColor !== "none") {
                newSegments.push({"path": currentPath, "color": currentColor});
            }
            root.trafficSegments = newSegments;
        }
    }

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
        var searchLimit = Math.min(routePoints.length - 1, 30);
        for (var i = 0; i < searchLimit; i++) {
            var d = distanceToSegment(carPos, routePoints[i], routePoints[i+1]);
            if (d < minDistance) minDistance = d;
        }
        if (minDistance > 75) recalculateRoute();
    }

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

    function searchDestination(address) {
        if (typeof mapboxApiKey === "undefined" || mapboxApiKey === "") return;
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
                    }
                } catch(e) {}
            }
        }
        http.send();
    }

    function requestSuggestions(query) {
        if (!query || query.length < 3 || typeof mapboxApiKey === "undefined" || mapboxApiKey === "") return;
        var http = new XMLHttpRequest();
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(query) + ".json?access_token=" + mapboxApiKey + "&autocomplete=true&limit=5&language=fr";
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
        enableSpeedZoom = true;
        map.center = QtPositioning.coordinate(carLat, carLon);
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

    // --- ANIMATIONS & LOGIQUE ---
    onCarLatChanged: {
        if (autoFollow) {
            map.center = QtPositioning.coordinate(carLat, carLon);
            if (enableSpeedZoom) {
                var targetZoom = 18;
                if (carSpeed > 100) targetZoom = 15;
                else if (carSpeed > 70) targetZoom = 16;
                else if (carSpeed > 40) targetZoom = 17;
                else targetZoom = 18;

                if (Math.abs(carZoom - targetZoom) > 0.5) {
                    internalZoomChange = true;
                    carZoom = targetZoom;
                    internalZoomChange = false;
                }
            }
            map.tilt = (carSpeed > 30) ? 45 : 0;
        }

        if (!isRecalculating) {
            updateRouteVisuals();
            checkIfOffRoute();
            updateTripStats();
            updateGuidance();
        }
    }

    onCarLonChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon); }

    onCarZoomChanged: {
        if (!internalZoomChange) {
            enableSpeedZoom = false;
        }
        if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon);
    }

    // --- INTERFACE UTILISATEUR ---
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
}
