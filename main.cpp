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
#include <QGeoServiceProvider>
#include "gpstelemetrysource.h" // <--- Ajouter l'include

int main(int argc, char *argv[]) {
    QLoggingCategory::setFilterRules(
        "qt.network.ssl.warning=true\n"
        "qt.location.mapping.osm.debug=true\n"
        "qt.location.mapping.network.debug=true\n"
        "qt.network.access.debug=true"
        );

    //QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    QApplication a(argc, argv);

    QString cachePath = QCoreApplication::applicationDirPath() + "/qtlocation_cache";
    QDir().mkpath(cachePath);
    qputenv("QTLOCATION_OSM_CACHE_DIR", cachePath.toUtf8()); // OK

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // 1. D�CLARATION INDISPENSABLE (� ne jamais commenter)
    TelemetryData telemetry;

#ifdef Q_OS_LINUX_PI
    // Code spécifique à la Pi (GPS réel sur port série)
    GpsTelemetrySource gpsSource(&telemetry);
    gpsSource.start("/dev/serial0");
#else
    // Code pour Windows (Simulation)
    MockTelemetrySource mock(&telemetry);
    mock.start();
#endif

    MainWindow w(&telemetry);
    w.showFullScreen();
    return a.exec();
}

