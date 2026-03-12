/**
 * @file main.cpp
 * @brief Point d'entr�e de l'application (Bootstrapper).
 * @details Ce fichier agit comme le chef d'orchestre du d�marrage. Ses responsabilit�s sont :
 * 1. Configurer l'environnement bas niveau de l'OS (Variables d'environnement Linux/Raspberry).
 * 2. Initialiser les moteurs de rendu lourd (OpenGL et Chromium/WebEngine).
 * 3. Appliquer le motif d'architecture "Injection de D�pendances" en instanciant le Mod�le
 * central et en le distribuant aux diff�rents Contr�leurs et Capteurs mat�riels.
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
    // --- 1. CONFIGURATION SYST�ME ET GRAPHIQUE ---

    // D�sactive le moteur de th�me externe pour garder le contr�le total sur le style sombre
    qputenv("QT_QPA_PLATFORMTHEME", "");

    // Indispensable pour que WebEngine (Chromium) fonctionne avec le GPU sous Wayland
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // Optimisations pour Chromium (utilis� dans la page HomeAssistant / Media)
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--disable-web-security "
            "--no-sandbox "
            "--ignore-certificate-errors "
            "--log-level=3 "
            "--disable-logging "
            "--enable-logging=none");

    // --- 2. INITIALISATION DE L'APPLICATION ---

    QApplication a(argc, argv); // L'objet 'a' doit �tre cr�� ICI pour �tre utilis� ensuite

    // --- 3. TH�ME SOMBRE GLOBAL (FEUILLE DE STYLE) ---
    // Cette section d�finit l'apparence de toute l'application pour �viter les fonds blancs
    a.setStyleSheet(
        // Fond de tous les widgets par d�faut
        "QWidget { "
        "   background-color: #171a21; "
        "   color: white; "
        "   font-family: 'Segoe UI', sans-serif;"
        "}"
        // Style des boutons (bas� sur votre charte graphique)
        "QPushButton { "
        "   background-color: #222634; "
        "   border-radius: 12px; "
        "   padding: 8px; "
        "   color: white; "
        "}"
        "QPushButton:pressed { background-color: #2d3245; }"
        // Style des listes (Bluetooth, etc.)
        "QAbstractItemView { "
        "   background-color: #2a2f3a; "
        "   alternate-background-color: #222634; "
        "   selection-background-color: #2a75ff; "
        "   border: none; "
        "   outline: none; "
        "}"
        // Style des barres de d�filement (Scrollbars)
        "QScrollBar:vertical { "
        "   border: none; background: #171a21; width: 10px; "
        "}"
        "QScrollBar::handle:vertical { background: #3e4452; border-radius: 5px; }"
        "QFrame { border: none; }"
        );

    // Utilisation du style Fusion comme base (tr�s flexible pour le mode sombre)
    QQuickStyle::setStyle("Fusion");

    // --- 4. CONFIGURATION DES LOGS ET R�SEAU ---

    QLoggingCategory::setFilterRules(
        "*.debug=false\n"
        "qt.qpa.*=false\n"
        "qt.text.font.*=false\n"
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=false\n"
        "qt.network.access.debug=false"
        );

    // Chemin des plugins pour les ic�nes SVG sur Raspberry Pi
    QCoreApplication::addLibraryPath("/usr/lib/aarch64-linux-gnu/qt6/plugins");

    // Cache local pour les cartes OSM (r�duit la data r�seau)
    QString cachePath = QCoreApplication::applicationDirPath() + "/qtlocation_cache";
    QDir().mkpath(cachePath);
    qputenv("QTLOCATION_OSM_CACHE_DIR", cachePath.toUtf8());

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // --- 5. ARCHITECTURE ET INJECTION DE D�PENDANCES ---

    // Le "Single Source of Truth" (Mod�le de donn�es central)
    TelemetryData telemetry;

    // Initialisation du GPS (Port S�rie)
    GpsTelemetrySource gpsSource(&telemetry);
#ifdef Q_OS_LINUX
    gpsSource.start("/dev/serial0");
#else
    gpsSource.start("COM1");
#endif

    // Initialisation de la Centrale Inertielle (IMU)
    Mpu9250Source mpuSource(&telemetry);
    mpuSource.start();

    // D�marrage de l'IHM avec injection de la t�l�m�trie
    MainWindow w(&telemetry);
    w.showFullScreen();

    // Lancement de la boucle d'�v�nements
    return a.exec();
}
