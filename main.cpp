/**
 * @file main.cpp
 * @brief Point d'entrée de l'application (Bootstrapper).
 * @details Ce fichier agit comme le chef d'orchestre du démarrage. Ses responsabilités sont :
 * 1. Configurer l'environnement bas niveau de l'OS (Variables d'environnement Linux/Raspberry).
 * 2. Initialiser les moteurs de rendu lourd (OpenGL et Chromium/WebEngine).
 * 3. Appliquer le motif d'architecture "Injection de Dépendances" en instanciant le Modèle
 * central et en le distribuant aux différents Contrôleurs et Capteurs matériels.
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
#include "mpu9250source.h"

int main(int argc, char *argv[]) {

    // --- 1. CONFIGURATION SYSTÈME ET GRAPHIQUE ---

    // Force Qt à utiliser le serveur d'affichage X11 (via le plugin xcb).
    qputenv("QT_QPA_PLATFORM", "xcb");

    // Activation du partage de contexte OpenGL entre les threads.
    // Prérequis obligatoire pour que le moteur Chromium (QWebEngine) puisse utiliser
    // l'accélération matérielle (GPU) du Raspberry Pi sans bloquer l'interface graphique.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // --- 2. SÉCURITÉ ET MOTEUR WEB (HOME ASSISTANT) ---
    // En environnement embarqué, l'écran interagit avec des serveurs locaux (ex: Home Assistant sur le même réseau).
    // Ces serveurs n'ont souvent pas de certificats HTTPS officiels.
    // On désactive intentionnellement certaines sécurités strictes de Chromium (CORS, Sandbox, Certificats)
    // pour garantir une communication fluide et autoriser l'autoplay des médias sans interaction tactile préalable.
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--disable-web-security "
            "--allow-running-insecure-content "
            "--autoplay-policy=no-user-gesture-required "
            "--ignore-certificate-errors "
            "--no-sandbox "
            "--disable-features=IsolateOrigins,site-per-process");

    // Standardisation du design des composants Qt Quick Controls 2
    QQuickStyle::setStyle("Fusion");

    // --- 3. DÉBOGAGE ET RÉSEAU ---
    // Filtres de logs activés pour surveiller spécifiquement le trafic de l'API cartographique (Mapbox/OSM)
    // et diagnostiquer les potentielles pertes de paquets en connexion mobile (4G partagée).
    QLoggingCategory::setFilterRules(
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=true\n"
        "qt.location.mapping.network.debug=true\n"
        "qt.network.access.debug=true"
        );

    // Démarrage du moteur événementiel (Event Loop) de Qt
    QApplication a(argc, argv);

    // --- 4. OPTIMISATION BANDE PASSANTE (CACHE CARTOGRAPHIQUE) ---
    // Pour éviter de retélécharger les mêmes tuiles Mapbox/OSM à chaque trajet,
    // on force la création d'un cache local persistant sur la carte SD du Raspberry Pi.
    // Cela réduit la latence d'affichage et économise la data réseau.
    QString cachePath = QCoreApplication::applicationDirPath() + "/qtlocation_cache";
    QDir().mkpath(cachePath);
    qputenv("QTLOCATION_OSM_CACHE_DIR", cachePath.toUtf8());

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // --- 5. ARCHITECTURE ET INJECTION DE DÉPENDANCES ---

    // A. Le "Single Source of Truth" (Modèle de données central)
    // Cet objet stocke l'état complet du véhicule en mémoire vive.
    TelemetryData telemetry;

    // B. Initialisation du GPS (Port Série / UART)
    // On injecte le pointeur '&telemetry' pour que le GPS puisse y écrire ses trames NMEA.
    GpsTelemetrySource gpsSource(&telemetry);
    gpsSource.start("/dev/serial0");

    // C. Initialisation de la Centrale Inertielle (Bus I2C)
    // Gère le gyroscope, l'accéléromètre et la boussole via le filtre de Madgwick.
    Mpu9250Source mpuSource(&telemetry);
    mpuSource.start();

    // D. Démarrage de l'Interface Homme-Machine (IHM)
    // La fenêtre principale reçoit la télémétrie en lecture seule pour l'afficher à l'écran.
    MainWindow w(&telemetry);
    w.show();

    // 6. Lancement de la boucle infinie (bloquante).
    return a.exec();
}
