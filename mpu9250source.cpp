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

void Mpu9250Source::start() {
    m_fileDescriptor = open("/dev/i2c-1", O_RDWR);
    if (m_fileDescriptor < 0) {
        qWarning() << "❌ Impossible d'ouvrir le bus I2C-1. Vérifiez l'activation I2C.";
        return;
    }

    // Réveil et configuration MPU9250
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char config[2] = {0x6B, 0x00}; // Wake up
    if (write(m_fileDescriptor, config, 2) != 2) {
        qWarning() << "❌ MPU9250 ne répond pas à l'adresse 0x68.";
        return;
    }

    // Activation Bypass pour voir le Magnétomètre
    config[0] = 0x37; config[1] = 0x02;
    write(m_fileDescriptor, config, 2);

    // Initialisation AK8963
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
    config[0] = 0x0A; config[1] = 0x16; // 16-bit, 100Hz
    if (write(m_fileDescriptor, config, 2) != 2) {
        qWarning() << "⚠️ Magnétomètre (0x0C) introuvable. Vérifiez AD0/CS.";
    }

    m_elapsedTimer.start();
    m_timer->start(50); // 20Hz est suffisant et plus stable pour le CPU
    qDebug() << "✅ Boussole démarrée.";
}

void Mpu9250Source::readSensor() {
    if (m_fileDescriptor < 0) return;

    float dt = m_elapsedTimer.restart() / 1000.0f;

    // 1. Lecture Accel/Gyro
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char reg = 0x3B;
    char dataAG[14];
    if (write(m_fileDescriptor, &reg, 1) == 1 && read(m_fileDescriptor, dataAG, 14) == 14) {
        float ax = (int16_t)((dataAG[0] << 8) | dataAG[1]) * (2.0f / 32768.0f);
        float ay = (int16_t)((dataAG[2] << 8) | dataAG[3]) * (2.0f / 32768.0f);
        float az = (int16_t)((dataAG[4] << 8) | dataAG[5]) * (2.0f / 32768.0f);
        float gx = (int16_t)((dataAG[8] << 8) | dataAG[9]) * (250.0f / 32768.0f);

        // 2. Lecture Magnétomètre
        ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
        char st1;
        char regSt1 = 0x02;
        write(m_fileDescriptor, &regSt1, 1);
        if (read(m_fileDescriptor, &st1, 1) == 1 && (st1 & 0x01)) {
            char regMag = 0x03;
            char dataM[7];
            write(m_fileDescriptor, &regMag, 1);
            if (read(m_fileDescriptor, dataM, 7) == 7) {
                float mx = (int16_t)((dataM[1] << 8) | dataM[0]);
                float my = (int16_t)((dataM[3] << 8) | dataM[2]);

                // Calcul direct du cap (Heading) en attendant une fusion parfaite
                float heading = std::atan2(my, mx) * (180.0 / M_PI);
                if (heading < 0) heading += 360.0f;

                qDebug() << "🧭 Cap boussole :" << heading;
                if (m_data) m_data->setHeading(static_cast<double>(heading));
            }
        }
    } else {
        qWarning() << "⚠️ Erreur de communication I2C. Vérifiez le câblage !";
    }
}
