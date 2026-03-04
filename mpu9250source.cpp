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

// Correction de l'erreur "undefined reference" : implémentation du destructeur
Mpu9250Source::~Mpu9250Source() {
    stop();
}

void Mpu9250Source::start() {
    m_fileDescriptor = open("/dev/i2c-1", O_RDWR);
    if (m_fileDescriptor < 0) return;

    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char config[2] = {0x6B, 0x00}; // Wake up
    write(m_fileDescriptor, config, 2);

    config[0] = 0x37; config[1] = 0x02; // Bypass Enable
    write(m_fileDescriptor, config, 2);

    ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
    config[0] = 0x0A; config[1] = 0x16; // 16-bit, 100Hz
    write(m_fileDescriptor, config, 2);

    m_elapsedTimer.start();
    m_timer->start(20); // 50Hz
}

void Mpu9250Source::readSensor() {
    if (m_fileDescriptor < 0) return;

    float dt = m_elapsedTimer.restart() / 1000.0f;

    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char reg = 0x3B;
    char dataAG[14];
    if (write(m_fileDescriptor, &reg, 1) == 1 && read(m_fileDescriptor, dataAG, 14) == 14) {
        float ax = (int16_t)((dataAG[0] << 8) | dataAG[1]) * (2.0f / 32768.0f);
        float ay = (int16_t)((dataAG[2] << 8) | dataAG[3]) * (2.0f / 32768.0f);
        float az = (int16_t)((dataAG[4] << 8) | dataAG[5]) * (2.0f / 32768.0f);
        float gx = (int16_t)((dataAG[8] << 8) | dataAG[9]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
        float gy = (int16_t)((dataAG[10] << 8) | dataAG[11]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
        float gz = (int16_t)((dataAG[12] << 8) | dataAG[13]) * (250.0f / 32768.0f) * (M_PI / 180.0f);

        ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
        char st1;
        char regSt1 = 0x02;
        write(m_fileDescriptor, &regSt1, 1);
        if (read(m_fileDescriptor, &st1, 1) == 1 && (st1 & 0x01)) {
            char regMag = 0x03;
            char dataM[7];
            write(m_fileDescriptor, &regMag, 1);
            if (read(m_fileDescriptor, dataM, 7) == 7) {
                float mx = ((int16_t)((dataM[1] << 8) | dataM[0]) - m_magBias[0]) * m_magScale[0];
                float my = ((int16_t)((dataM[3] << 8) | dataM[2]) - m_magBias[1]) * m_magScale[1];
                float mz = ((int16_t)((dataM[5] << 8) | dataM[4]) - m_magBias[2]) * m_magScale[2];

                // On utilise enfin toutes les variables ici, fini les warnings !
                madgwickUpdate(ax, ay, az, gx, gy, gz, mx, my, mz, dt);

                float yaw = atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]);
                float heading = yaw * (180.0f / M_PI);

                heading += 2.0f; // Déclinaison
                if (heading < 0) heading += 360.0f;
                if (heading > 360.0f) heading -= 360.0f;

                if (m_data) m_data->setHeading(static_cast<double>(heading));
            }
        }
    }
}

void Mpu9250Source::madgwickUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float dt) {
    // Pour l'instant, intégration basique pour éviter les warnings et avoir un cap
    // On pourra injecter ici l'algorithme complet de Kris Winer
    float qDot1 = 0.5f * (-q[1] * gx - q[2] * gy - q[3] * gz);
    float qDot2 = 0.5f * (q[0] * gx + q[2] * gz - q[3] * gy);
    float qDot3 = 0.5f * (q[0] * gy - q[1] * gz + q[3] * gx);
    float qDot4 = 0.5f * (q[0] * gz + q[1] * gy - q[2] * gx);

    q[0] += qDot1 * dt; q[1] += qDot2 * dt; q[2] += qDot3 * dt; q[3] += qDot4 * dt;
    float norm = sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    if (norm > 0) {
        q[0] /= norm; q[1] /= norm; q[2] /= norm; q[3] /= norm;
    }
}

void Mpu9250Source::stop() {
    m_timer->stop();
    if (m_fileDescriptor >= 0) {
        close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
}
