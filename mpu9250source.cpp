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

    // Tentative d'accès au magnétomètre
    if (ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C) < 0) {
        qWarning() << "⚠️ Boussole perdue... Tentative de réinitialisation du Bypass.";

        // Si on perd le magnétomètre, on repasse sur l'adresse du MPU (0x68)
        // pour réactiver le bypass et le réveil.
        ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
        char wakeUp[2] = {0x6B, 0x00};
        write(m_fileDescriptor, wakeUp, 2);
        char bypass[2] = {0x37, 0x02};
        write(m_fileDescriptor, bypass, 2);

        // On réinitialise aussi le magnétomètre
        ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
        char magConfig[2] = {0x0A, 0x16};
        write(m_fileDescriptor, magConfig, 2);
        return;
    }

    // 2. Vérifier si une donnée est prête (Registre ST1 - 0x02)
    char st1;
    char regSt1[1] = {0x02};
    if (write(m_fileDescriptor, regSt1, 1) != 1) return;
    if (read(m_fileDescriptor, &st1, 1) != 1) return;

    if (!(st1 & 0x01)) {
        // La donnée n'est pas encore prête, on sort poliment
        return;
    }

    // 3. Lire les 7 octets (6 de données + 1 de statut ST2 indispensable !)
    char regMag[1] = {0x03};
    if (write(m_fileDescriptor, regMag, 1) != 1) return;

    char magData[7];
    int bytesRead = read(m_fileDescriptor, magData, 7);

    if (bytesRead >= 7) {
        // Vérification du débordement magnétique dans ST2 (bit 3)
        if (!(magData[6] & 0x08)) {
            int16_t mx = (magData[1] << 8) | magData[0];
            int16_t my = (magData[3] << 8) | magData[2];

            // Calcul du cap
            float heading = std::atan2(static_cast<float>(my), static_cast<float>(mx)) * (180.0 / M_PI);
            if (heading < 0) heading += 360;

            // Filtre de lissage pour éviter que la carte ne tremble
            static double filteredHeading = 0;
            filteredHeading = (filteredHeading * 0.7) + (heading * 0.3);

            qDebug() << "Boussole stable :" << filteredHeading << "°";

            if (m_data) {
                m_data->setHeading(filteredHeading);
            }
        }
    } else {
        qWarning() << "MPU9250: Erreur de lecture I2C (reçu" << bytesRead << "octets)";
    }
}

void Mpu9250Source::stop() {
    m_timer->stop();
    if (m_fileDescriptor >= 0) {
        close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
}
