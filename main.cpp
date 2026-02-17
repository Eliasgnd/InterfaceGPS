#include <QApplication>
#include <QLoggingCategory>
#include "mainwindow.h"
#include "telemetrydata.h"
#include "gpstelemetrysource.h"
#include <QNetworkProxyFactory>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    // 1. FORCE LE MODE X11 (Pour contourner les restrictions Wayland sur le clavier)
    qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));

    // 2. ACTIVE LE PLUGIN DE CLAVIER AU NIVEAU SYSTÈME
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    // 3. FORCE LE CLAVIER À RESTER DANS LA FENÊTRE (Style embarqué)
    qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", QByteArray("1"));

    // Optionnel : Style sombre pour voiture
    qputenv("QT_VIRTUALKEYBOARD_STYLE", QByteArray("default"));

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
    GpsTelemetrySource gpsSource(&telemetry);
    gpsSource.start("/dev/serial0");

    MainWindow w(&telemetry);
    w.showFullScreen();

    return a.exec();
}
