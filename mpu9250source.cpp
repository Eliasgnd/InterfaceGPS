#include "mpu9250source.h"
#include "telemetrydata.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cmath>

Mpu9250Source::Mpu9250Source(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data), m_fileDescriptor(-1) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Mpu9250Source::readSensor);
}

Mpu9250Source::~Mpu9250Source() { stop(); }

void Mpu9250Source::start() {
    m_fileDescriptor = open("/dev/i2c-1", O_RDWR);
    if (m_fileDescriptor < 0) return;

    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char config[2] = {0x6B, 0x00}; write(m_fileDescriptor, config, 2); // Wake up
    config[0] = 0x37; config[1] = 0x02; write(m_fileDescriptor, config, 2); // Bypass

    ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
    config[0] = 0x0A; config[1] = 0x16; write(m_fileDescriptor, config, 2); // 16-bit, 100Hz

    m_elapsedTimer.start();
    m_timer->start(20);
    qDebug() << "🛰️ MODE CALIBRATION ACTIF : Faites des '8' avec le capteur !";
}

void Mpu9250Source::readSensor() {
    if (m_fileDescriptor < 0) return;
    float dt = m_elapsedTimer.restart() / 1000.0f;

    // Lecture Mag uniquement pour la calibration
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
    char regSt1 = 0x02; char st1;
    write(m_fileDescriptor, &regSt1, 1); read(m_fileDescriptor, &st1, 1);

    if (st1 & 0x01) {
        char regMag = 0x03; char dataM[7];
        write(m_fileDescriptor, &regMag, 1);
        if (read(m_fileDescriptor, dataM, 7) == 7) {
            int16_t mx = (int16_t)((dataM[1] << 8) | dataM[0]);
            int16_t my = (int16_t)((dataM[3] << 8) | dataM[2]);
            int16_t mz = (int16_t)((dataM[5] << 8) | dataM[4]);

            // Mise à jour des Min/Max
            if (mx < m_magMin[0]) m_magMin[0] = mx; if (mx > m_magMax[0]) m_magMax[0] = mx;
            if (my < m_magMin[1]) m_magMin[1] = my; if (my > m_magMax[1]) m_magMax[1] = my;
            if (mz < m_magMin[2]) m_magMin[2] = mz; if (mz > m_magMax[2]) m_magMax[2] = mz;

            m_calibrationCounter++;

            // Toutes les 50 lectures (1 seconde), on affiche les coefficients calculés
            if (m_calibrationCounter % 50 == 0) {
                float bX = (float)(m_magMax[0] + m_magMin[0]) / 2.0f;
                float bY = (float)(m_magMax[1] + m_magMin[1]) / 2.0f;
                float bZ = (float)(m_magMax[2] + m_magMin[2]) / 2.0f;

                float cX = (float)(m_magMax[0] - m_magMin[0]) / 2.0f;
                float cY = (float)(m_magMax[1] - m_magMin[1]) / 2.0f;
                float cZ = (float)(m_magMax[2] - m_magMin[2]) / 2.0f;
                float avg = (cX + cY + cZ) / 3.0f;

                qDebug() << "--- COPIEZ CES LIGNES DANS VOTRE CODE FINAL ---";
                qDebug() << QString("m_magBias[0] = %1f; m_magBias[1] = %2f; m_magBias[2] = %3f;").arg(bX).arg(bY).arg(bZ);
                qDebug() << QString("m_magScale[0] = %1f; m_magScale[1] = %2f; m_magScale[2] = %3f;").arg(avg/cX).arg(avg/cY).arg(avg/cZ);
                qDebug() << "-----------------------------------------------";
            }
        }
    }
}

void Mpu9250Source::stop() {
    m_timer->stop();
    if (m_fileDescriptor >= 0) { close(m_fileDescriptor); m_fileDescriptor = -1; }
}

void Mpu9250Source::madgwickUpdate(float, float, float, float, float, float, float, float, float, float) {}
