#include "mpu9250source.h"
#include "telemetrydata.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

Mpu9250Source::Mpu9250Source(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data), m_fileDescriptor(-1) {
    m_timer = new QTimer(this);
    // On lit le capteur toutes les 200 millisecondes
    connect(m_timer, &QTimer::timeout, this, &Mpu9250Source::readSensor);
}

Mpu9250Source::~Mpu9250Source() {
    stop();
}

void Mpu9250Source::start() {
    // Ouverture du bus I2C 1 de la Raspberry Pi
    m_fileDescriptor = open("/dev/i2c-1", O_RDWR);
    if (m_fileDescriptor < 0) {
        qWarning() << "MPU9250: Impossible d'ouvrir le bus I2C.";
        return;
    }

    // Connexion à l'adresse I2C du MPU9250 (généralement 0x68)
    if (ioctl(m_fileDescriptor, I2C_SLAVE, 0x68) < 0) {
        qWarning() << "MPU9250: Impossible de trouver le capteur à l'adresse 0x68.";
        return;
    }

    // Réveil du capteur (écrire 0 dans le registre PWR_MGMT_1)
    char wakeUp[2] = {0x6B, 0x00};
    write(m_fileDescriptor, wakeUp, 2);

    m_timer->start(200);
    qDebug() << "✅ MPU9250 Démarré !";
}

void Mpu9250Source::stop() {
    m_timer->stop();
    if (m_fileDescriptor >= 0) {
        close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
}

void Mpu9250Source::readSensor() {
    if (m_fileDescriptor < 0) return;

    // Registre de départ pour lire l'accéléromètre, température, puis gyroscope
    char reg[1] = {0x3B};
    write(m_fileDescriptor, reg, 1);

    char data[14];
    if (read(m_fileDescriptor, data, 14) == 14) {
        // En premier temps, on affiche juste l'accéléromètre X, Y, Z pour vérifier que ça marche
        int16_t accelX = (data[0] << 8) | data[1];
        int16_t accelY = (data[2] << 8) | data[3];
        int16_t accelZ = (data[4] << 8) | data[5];

        qDebug() << "MPU9250 Brut -> Accel X:" << accelX << " Y:" << accelY << " Z:" << accelZ;

        // Plus tard, vous calculerez le vrai "Cap" (Heading) via le magnétomètre (AK8963)
        // et vous pourrez l'injecter dans la carte avec :
        // if (m_data) m_data->setHeading(valeur_en_degres);
    }
}
