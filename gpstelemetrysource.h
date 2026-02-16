#pragma once
#include <QObject>
#include <QSerialPort>
#include <QNmeaPositionInfoSource> // <--- Le moteur GPS de Qt
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
    // On remplace onReadyRead par ce slot officiel de Qt Positioning
    void onPositionUpdated(const QGeoPositionInfo &info);

private:
    TelemetryData* m_data = nullptr;
    QSerialPort* m_serial = nullptr;
    QNmeaPositionInfoSource* m_nmeaSource = nullptr; // <--- L'objet magique
};
