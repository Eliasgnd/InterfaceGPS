#include <QApplication>
#include <QLoggingCategory> // <--- AJOUTER CECI
#include "mainwindow.h"
#include "telemetrydata.h"
#include "mocktelemetrysource.h"
#include <QNetworkProxyFactory>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    QLoggingCategory::setFilterRules(
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=true\n"
        "qt.location.mapping.network.debug=true\n"
        "qt.network.access.debug=true"
        );

    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    QApplication a(argc, argv);

    QString cachePath = QCoreApplication::applicationDirPath() + "/qtlocation_cache";
    QDir().mkpath(cachePath);
    qputenv("QTLOCATION_OSM_CACHE_DIR", cachePath.toUtf8()); // OK

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    TelemetryData telemetry;
    MockTelemetrySource mock(&telemetry);
    mock.start();

    MainWindow w(&telemetry);
    w.showFullScreen();
    return a.exec();
}

