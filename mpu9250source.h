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
    // Fonctions de calcul
    void madgwickUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float dt);

    // Paramètres matériels
    TelemetryData* m_data;
    QTimer* m_timer;
    QElapsedTimer m_elapsedTimer;
    int m_fileDescriptor;

    // État du filtre (Quaternions)
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    const float beta = 0.1f; // Gain du filtre (ajustable entre 0.01 et 0.5)

    // --- PARAMÈTRES DE CALIBRATION (À ajuster après vos tests) ---
    // Hard Iron (Offsets)
    float m_magBias[3] = {0.0f, 0.0f, 0.0f};
    // Soft Iron (Scaling)
    float m_magScale[3] = {1.0f, 1.0f, 1.0f};
};

#endif // MPU9250SOURCE_H
