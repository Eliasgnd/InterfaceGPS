#include "mpu9250source.h"
#include "telemetrydata.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cmath> // Nécessaire pour atan2 et M_PI

Mpu9250Source::Mpu9250Source(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data), m_fileDescriptor(-1) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Mpu9250Source::readSensor);
}

Mpu9250Source::~Mpu9250Source() {
    stop();
}

void Mpu9250Source::start() {
    m_fileDescriptor = open("/dev/i2c-1", O_RDWR);
    if (m_fileDescriptor < 0) return;

    // 1. Initialiser le MPU9250 (Réveil)
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char wakeUp[2] = {0x6B, 0x00};
    write(m_fileDescriptor, wakeUp, 2);

    // 2. Activer le mode "Bypass" pour voir le magnétomètre (AK8963)
    // On écrit 0x02 dans le registre INT_PIN_CFG (0x37)
    char bypass[2] = {0x37, 0x02};
    write(m_fileDescriptor, bypass, 2);

    // 3. Initialiser le Magnétomètre (Adresse 0x0C)
    if (ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C) >= 0) {
        // Mode de mesure continue 2 (100Hz) et résolution 16 bits
        char magConfig[2] = {0x0A, 0x16};
        write(m_fileDescriptor, magConfig, 2);
    }

    m_timer->start(100); // Lecture 10 fois par seconde pour une carte fluide
}

void Mpu9250Source::readSensor() {
    if (m_fileDescriptor < 0) return;

    // --- LECTURE DU MAGNÉTOMÈTRE ---
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);

    char regMag[1] = {0x03}; // Registre de début des données X,Y,Z
    write(m_fileDescriptor, regMag, 1);

    char magData[7];
    if (read(m_fileDescriptor, magData, 7) >= 6) {
        // Note : Le AK8963 est en Little Endian (octet faible en premier)
        int16_t mx = (magData[1] << 8) | magData[0];
        int16_t my = (magData[3] << 8) | magData[2];
        int16_t mz = (magData[5] << 8) | magData[4];

        // Calcul du Cap (Heading) en degrés
        // On utilise l'axe X et Y si le capteur est à plat
        float heading = std::atan2(static_cast<float>(my), static_cast<float>(mx)) * (180.0 / M_PI);

        // Normalisation entre 0 et 360°
        if (heading < 0) heading += 360;

        qDebug() << "Orientation Boussole :" << heading << "degrés";

        // Injection dans la télémétrie pour mettre à jour la carte
        if (m_data) {
            m_data->setHeading(static_cast<double>(heading));
        }
    }
}

void Mpu9250Source::stop() {
    m_timer->stop();
    if (m_fileDescriptor >= 0) {
        close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
}
