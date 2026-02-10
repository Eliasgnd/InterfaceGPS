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
    // Communication via String (JSON) pour éviter les bugs de types
    signal suggestionsUpdated(string suggestions)

    Plugin {
        id: mapPlugin
        name: "osm"

        // 1. TUILES (Affichage Carte via Mapbox)
        PluginParameter {
            name: "osm.mapping.custom.host"
            value: "https://api.mapbox.com/styles/v1/mapbox/streets-v12/tiles/512/%z/%x/%y@2x?access_token=" + mapboxApiKey
        }
        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }

        // 2. CACHE AVANCÉ (Pour éviter de retélécharger)
        // Stockage de 2 Go sur le disque (au lieu de 50 Mo par défaut)
        PluginParameter { name: "osm.mapping.cache.disk.size"; value: "2000000000" }
        // Durée de conservation : 1 an (525600 minutes) au lieu de 12h
        PluginParameter { name: "osm.mapping.cache.directory.expire"; value: "525600" }

        // 3. ROUTING (Itinéraire via serveur communautaire Allemand)
        // Ce serveur est fiable, gratuit et ne demande pas d'authentification complexe
        PluginParameter {
            name: "osm.routing.host";
            value: "https://routing.openstreetmap.de/routed-car/route/v1/driving/"
        }
        PluginParameter { name: "osm.routing.apiversion"; value: "v5" }
        PluginParameter { name: "osm.useragent"; value: "InterfaceGPS_Student/1.0" }
    }

    // Modèle pour l'itinéraire (Route)
    GeocodeModel {
        id: geocodeModel
        plugin: mapPlugin
        limit: 1
        onLocationsChanged: {
            // Ce bloc est moins utilisé car on passe par searchDestination manuelle ci-dessous
            if (status !== GeocodeModel.Ready || count <= 0) return
            var loc = get(0).coordinate
            routeQuery.clearWaypoints()
            routeQuery.addWaypoint(QtPositioning.coordinate(carLat, carLon))
            routeQuery.addWaypoint(loc)
            routeModel.update()
        }
    }

    RouteQuery { id: routeQuery }

    RouteModel {
        id: routeModel
        plugin: mapPlugin
        query: routeQuery
        autoUpdate: false

        onStatusChanged: {
            console.log("QML: RouteModel Status = " + status)

            if (status === RouteModel.Ready) {
                console.log("QML: RouteModel PRÊT ! Itinéraires trouvés : " + count)
                if (count > 0) {
                    var route = get(0);
                    map.fitViewport(route.path);
                    root.autoFollow = false;

                    var d = (route.distance / 1000).toFixed(1) + " km";
                    var t = Math.round(route.travelTime / 60) + " min";
                    console.log("QML: Infos route : " + d + " en " + t)
                    routeInfoUpdated(d, t);
                }
            } else if (status === RouteModel.Loading) {
                console.log("QML: Calcul en cours...")
            } else if (status === RouteModel.Error) {
                console.log("QML: ERREUR ROUTE : " + errorString)
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
                }
                transform: Rotation { origin.x: 30; origin.y: 30; angle: carHeading }
            }
        }
    }

    // --- FONCTIONS APPELÉES PAR LE C++ ---

    // 1. Recherche d'itinéraire (Bouton Aller)
    function searchDestination(address) {
        console.log("QML: Recherche destination vers : " + address)

        // Etape 1 : On demande à Mapbox où se trouve l'adresse (Geocoding précis)
        var http = new XMLHttpRequest()
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(address) + ".json?access_token=" + mapboxApiKey + "&limit=1"

        http.open("GET", url, true);
        http.onreadystatechange = function() {
            if (http.readyState == 4 && http.status == 200) {
                try {
                    var json = JSON.parse(http.responseText)
                    if (json.features && json.features.length > 0) {
                        var destLon = json.features[0].center[0]
                        var destLat = json.features[0].center[1]
                        console.log("QML: Coordonnées trouvées : " + destLat + ", " + destLon)

                        // Etape 2 : On lance le calcul de route (OSRM Allemand)
                        routeQuery.clearWaypoints()
                        routeQuery.addWaypoint(QtPositioning.coordinate(carLat, carLon)) // Départ
                        routeQuery.addWaypoint(QtPositioning.coordinate(destLat, destLon)) // Arrivée
                        routeModel.update()
                    } else {
                        console.log("QML: Adresse introuvable.")
                    }
                } catch (e) {
                    console.log("QML: Erreur JSON : " + e)
                }
            }
        }
        http.send();
    }

    // 2. Suggestions de recherche (Quand on tape)
    function requestSuggestions(query) {
        if (!query || query.length < 3) {
            root.suggestionsUpdated("[]")
            return
        }

        // On interroge Mapbox Geocoding avec Fuzzy Match
        var http = new XMLHttpRequest()
        var url = "https://api.mapbox.com/geocoding/v5/mapbox.places/" + encodeURIComponent(query) + ".json?access_token=" + mapboxApiKey + "&autocomplete=true&fuzzyMatch=true&language=fr&limit=5"

        http.open("GET", url, true);
        http.onreadystatechange = function() {
            if (http.readyState == 4 && http.status == 200) {
                try {
                    var json = JSON.parse(http.responseText)
                    var results = []
                    if (json.features) {
                        for (var i = 0; i < json.features.length; i++) {
                            results.push(json.features[i].place_name)
                        }
                    }
                    // Envoi au C++ sous forme de String JSON
                    root.suggestionsUpdated(JSON.stringify(results))
                } catch (e) {
                    console.log("QML: Erreur parsing suggestions")
                }
            }
        }
        http.send();
    }

    onCarLatChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }
    onCarLonChanged: { if (autoFollow) map.center = QtPositioning.coordinate(carLat, carLon) }
    function recenterMap() { autoFollow = true; map.center = QtPositioning.coordinate(carLat, carLon); map.zoomLevel = 17; }
}
