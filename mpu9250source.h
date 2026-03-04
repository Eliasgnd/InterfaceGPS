#ifndef MPU9250SOURCE_H
#define MPU9250SOURCE_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

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
    void madgwickUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float dt);

    TelemetryData* m_data;
    QTimer* m_timer;
    QElapsedTimer m_elapsedTimer;
    int m_fileDescriptor;

    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};

    // --- VARIABLES DE CALIBRATION ---
    float m_magBias[3] = {0.0f, 0.0f, 0.0f};
    float m_magScale[3] = {1.0f, 1.0f, 1.0f};

    // Valeurs pour la recherche des coefficients
    int16_t m_magMin[3] = {32767, 32767, 32767};
    int16_t m_magMax[3] = {-32768, -32768, -32768};
    long m_calibrationCounter = 0;
};

#endif
