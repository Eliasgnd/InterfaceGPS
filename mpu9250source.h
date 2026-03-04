#ifndef MPU9250SOURCE_H
#define MPU9250SOURCE_H

#include <QObject>
#include <QTimer>

class TelemetryData;

class Mpu9250Source : public QObject {
    Q_OBJECT
public:
    explicit Mpu9250Source(TelemetryData* data, QObject* parent = nullptr);
    ~Mpu9250Source();

    void start();
    void stop();

private slots:
    void readSensor();

private:
    TelemetryData* m_data;
    QTimer* m_timer;
    int m_fileDescriptor;
};

#endif // MPU9250SOURCE_H
