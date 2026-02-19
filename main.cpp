#include <QApplication>
#include <QLoggingCategory>
#include <QQuickStyle>
#include "mainwindow.h"
#include "telemetrydata.h"
#include "gpstelemetrysource.h"
#include <QNetworkProxyFactory>
#include <QDir>
#include <QCoreApplication>

int main(int argc, char *argv[]) {

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--disable-web-security "
            "--allow-running-insecure-content "
            "--autoplay-policy=no-user-gesture-required "
            "--ignore-certificate-errors "
            "--no-sandbox "
            "--disable-features=IsolateOrigins,site-per-process");

    // Force le style compatible avec la personnalisation QML
    QQuickStyle::setStyle("Fusion");

    QLoggingCategory::setFilterRules(
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=true\n"
        "qt.location.mapping.network.debug=true\n"
        "qt.network.access.debug=true"
        );

    QApplication a(argc, argv);

    // Configuration du cache pour les cartes
    QString cachePath = QCoreApplication::applicationDirPath() + "/qtlocation_cache";
    QDir().mkpath(cachePath);
    qputenv("QTLOCATION_OSM_CACHE_DIR", cachePath.toUtf8());

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    TelemetryData telemetry;
    GpsTelemetrySource gpsSource(&telemetry);
    gpsSource.start("/dev/serial0");

    MainWindow w(&telemetry);
    w.show();

    return a.exec();
}
