/**
 * @file main.cpp
 * @brief Rôle architectural : Point d'entrée de l'application embarquée InterfaceGPS.
 * @details Responsabilités : Initialiser le framework Qt, configurer le runtime
 * (moteur WebEngine, cache cartographique, logs réseau) et assembler les briques
 * principales de l'application (Télémétrie, Matériel, Interface utilisateur).
 * Dépendances principales : QApplication, QQuickStyle, QWebEngineCore et MainWindow.
 */

#include <QApplication>
#include <QLoggingCategory>
#include <QQuickStyle>
#include "mainwindow.h"
#include "telemetrydata.h"
#include "gpstelemetrysource.h"
#include <QNetworkProxyFactory>
#include <QDir>
#include <QCoreApplication>

/**
 * @brief Fonction principale (Point d'entrée C++).
 * @param argc Nombre d'arguments passés en ligne de commande.
 * @param argv Tableau des arguments de la ligne de commande.
 * @return Code de sortie de l'application (0 si succès).
 */
int main(int argc, char *argv[]) {

    // Optimisation pour QWebEngine : Permet le partage de contexte OpenGL entre les différents threads
    // et fenêtres pour un rendu matériel plus fluide. Doit être appelé avant l'instanciation de QApplication.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // --- CONFIGURATION CHROMIUM (WEBENGINE) ---
    // Ces flags assouplissent drastiquement les règles de sécurité du navigateur intégré.
    // Indispensable dans un contexte embarqué pour afficher une interface Home Assistant locale
    // qui n'a pas de certificat HTTPS valide, ou pour forcer la lecture automatique de médias.
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--disable-web-security "
            "--allow-running-insecure-content "
            "--autoplay-policy=no-user-gesture-required "
            "--ignore-certificate-errors "
            "--no-sandbox "
            "--disable-features=IsolateOrigins,site-per-process");

    // Choix du style graphique de base pour les éléments QML/Widgets
    QQuickStyle::setStyle("Fusion");

    // Configuration des logs de débogage pour surveiller les requêtes cartographiques et SSL
    QLoggingCategory::setFilterRules(
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=true\n"
        "qt.location.mapping.network.debug=true\n"
        "qt.network.access.debug=true"
        );

    // Initialisation de la boucle d'événements principale Qt
    QApplication a(argc, argv);

    // --- CONFIGURATION DU CACHE CARTOGRAPHIQUE ---
    // On force la création d'un dossier local pour mettre en cache les tuiles (images) de la carte.
    // Cela permet d'économiser de la data 4G/Wifi et d'accélérer l'affichage des zones déjà visitées.
    QString cachePath = QCoreApplication::applicationDirPath() + "/qtlocation_cache";
    QDir().mkpath(cachePath);
    qputenv("QTLOCATION_OSM_CACHE_DIR", cachePath.toUtf8());

    // Utilisation automatique du proxy réseau du système d'exploitation si configuré
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // --- ASSEMBLAGE DES COMPOSANTS (Dependency Injection) ---

    // 1. Instanciation du modèle de données central (Le Bus de l'application)
    TelemetryData telemetry;

    // 2. Initialisation et démarrage du capteur GPS matériel (écrit sur le Bus)
    // "/dev/serial0" est le port série matériel par défaut sur un Raspberry Pi
    GpsTelemetrySource gpsSource(&telemetry);
    gpsSource.start("/dev/serial0");

    // 3. Instanciation et affichage de l'Interface Graphique (lit le Bus)
    MainWindow w(&telemetry);
    w.show();

    // Démarrage de l'application (boucle infinie attendant les événements clavier/souris/réseau)
    return a.exec();
}
