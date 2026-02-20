// Rôle architectural: source de télémétrie GPS branchée sur un flux NMEA série.
// Responsabilités: démarrer/arrêter la lecture et publier les mises à jour de position vers TelemetryData.
// Dépendances principales: QSerialPort, QNmeaPositionInfoSource et QGeoPositionInfo.

#pragma once
#include <QObject>
#include <QSerialPort>
#include <QNmeaPositionInfoSource>
#include <QGeoPositionInfo>

class TelemetryData;

class GpsTelemetrySource : public QObject {
    Q_OBJECT
public:
    explicit GpsTelemetrySource(TelemetryData* data, QObject* parent = nullptr);
    ~GpsTelemetrySource();

    void start(const QString& portName = "/dev/serial0");
    void stop();

private slots:

    void onPositionUpdated(const QGeoPositionInfo &info);

private:
    TelemetryData* m_data = nullptr;
    QSerialPort* m_serial = nullptr;
    QNmeaPositionInfoSource* m_nmeaSource = nullptr;
};
