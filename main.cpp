#include <QApplication>
#include <QLoggingCategory>
#include "mainwindow.h"
#include "telemetrydata.h"
#include "gpstelemetrysource.h" // N'oubliez pas cet include
#include <QNetworkProxyFactory>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    // 1. ACTIVE LE CLAVIER VIRTUEL
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    // 2. FORCE LE CLAVIER À RESTER COLLÉ EN BAS (Mode "Embedded")
    // Sans cette ligne, il flotte comme une fenêtre sur le bureau du Pi
    //qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", QByteArray("1"));

    // --- Reste de votre configuration existante ---
    QLoggingCategory::setFilterRules(
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=true\n"
        "qt.location.mapping.network.debug=true\n"
        "qt.network.access.debug=true"
        );

    QApplication a(argc, argv);

    QString cachePath = QCoreApplication::applicationDirPath() + "/qtlocation_cache";
    QDir().mkpath(cachePath);
    qputenv("QTLOCATION_OSM_CACHE_DIR", cachePath.toUtf8());

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    TelemetryData telemetry;

    // Source GPS réelle
    GpsTelemetrySource gpsSource(&telemetry);
    gpsSource.start("/dev/serial0");

    MainWindow w(&telemetry);
    w.showFullScreen();

    return a.exec();
}
