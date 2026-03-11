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
    qputenv("QT_QPA_PLATFORMTHEME", ""); // D�sactive le moteur de th�me externe

    QLoggingCategory::setFilterRules(
        "*.debug=false\n"
        "qt.qpa.*=false\n"
        "qt.text.font.*=false\n"
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=false\n" // Mis � false pour nettoyer
        "qt.network.access.debug=false"          // Mis � false pour nettoyer
        );

    // --- 1. CONFIGURATION SYSTÈME ET GRAPHIQUE ---

    // Gardez le chemin des plugins pour les icônes SVG
    QCoreApplication::addLibraryPath("/usr/lib/aarch64-linux-gnu/qt6/plugins");

#ifdef Q_OS_LINUX
    // SUPPRIMEZ OU COMMENTEZ CETTE LIGNE :
    // qputenv("QT_QPA_PLATFORM", "xcb");

    // Sous Wayland, Qt choisira automatiquement 'wayland' ou 'eglfs'.
    // Si l'app ne se lance pas, vous pouvez tester de forcer :
    // qputenv("QT_QPA_PLATFORM", "wayland");
#endif

    // Indispensable pour que WebEngine (Chromium) fonctionne avec le GPU sous Wayland
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // Vous pouvez essayer de réactiver l'accélération GPU pour Chromium
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--disable-web-security "
            "--no-sandbox "
            "--ignore-certificate-errors "
            "--log-level=3 "
            "--disable-logging "
            "--enable-logging=none");

    QQuickStyle::setStyle("Fusion");

    // --- 3. DÉBOGAGE ET RÉSEAU ---

    // Démarrage du moteur événementiel (Event Loop) de Qt
    QApplication a(argc, argv);

    // --- 4. OPTIMISATION BANDE PASSANTE (CACHE CARTOGRAPHIQUE) ---
    // Pour éviter de retélécharger les mêmes tuiles Mapbox/OSM à chaque trajet,
    // on force la création d'un cache local persistant.
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
#ifdef Q_OS_LINUX
    gpsSource.start("/dev/serial0"); // Chemin Linux/Raspberry
#else
    gpsSource.start("COM1"); // Port factice ou de test pour Windows
#endif

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
