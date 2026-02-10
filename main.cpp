#include <QApplication>
#include "mainwindow.h"
#include "telemetrydata.h"
#include "mocktelemetrysource.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    TelemetryData telemetry;
    MockTelemetrySource mock(&telemetry);
    mock.start();

    MainWindow w(&telemetry);
    w.showFullScreen();   // tactile
    return a.exec();
}
