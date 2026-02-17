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

    // 1. On active le module de clavier
        qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

        // 2. IMPORTANT : On désactive l'intégration au bureau Linux
        // Cela force le clavier à être un "item" QML à l'intérieur de l'app
        qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", QByteArray("1"));

        // On ne force PAS xcb ici pour laisser Qt6 gérer le rendu natif
        // qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));

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
