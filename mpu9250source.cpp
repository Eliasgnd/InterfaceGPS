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

Mpu9250Source::~Mpu9250Source() {
    stop();
}

void Mpu9250Source::start() {
    qDebug() << "🔍 Tentative d'ouverture du bus I2C-1...";
    m_fileDescriptor = open("/dev/i2c-1", O_RDWR);
    if (m_fileDescriptor < 0) return;

    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char config[2] = {0x6B, 0x00}; write(m_fileDescriptor, config, 2); // Wake up

    config[0] = 0x1C; config[1] = 0x00; write(m_fileDescriptor, config, 2); // Accel +/- 2g
    config[0] = 0x1B; config[1] = 0x00; write(m_fileDescriptor, config, 2); // Gyro 250 dps

    // --- AUTO-CALIBRATION DU GYROSCOPE (Kris Winer Method) ---
    qDebug() << "⏳ Calibration du Gyroscope (NE BOUGEZ PAS LE CAPTEUR)...";
    float gyroSum[3] = {0, 0, 0};
    int samples = 200;
    char reg = 0x43; // Registre de départ du Gyro
    char dataG[6];

    for (int i = 0; i < samples; i++) {
        write(m_fileDescriptor, &reg, 1);
        if (read(m_fileDescriptor, dataG, 6) == 6) {
            gyroSum[0] += (int16_t)((dataG[0] << 8) | dataG[1]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
            gyroSum[1] += (int16_t)((dataG[2] << 8) | dataG[3]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
            gyroSum[2] += (int16_t)((dataG[4] << 8) | dataG[5]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
        }
        usleep(5000); // Pause de 5ms (donc 200 lectures = 1 seconde)
    }
    m_gyroBias[0] = gyroSum[0] / samples;
    m_gyroBias[1] = gyroSum[1] / samples;
    m_gyroBias[2] = gyroSum[2] / samples;
    qDebug() << "✅ Gyroscope calibré. Biais retirés.";
    // -----------------------------------------------------------

    // Activation Bypass pour Magnétomètre
    config[0] = 0x37; config[1] = 0x02; write(m_fileDescriptor, config, 2);
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);
    config[0] = 0x0A; config[1] = 0x16; write(m_fileDescriptor, config, 2);

    m_elapsedTimer.start();
    m_timer->start(100);
    qDebug() << "🚀 Mpu9250Source démarré avec succès.";
}

void Mpu9250Source::readSensor() {
    if (m_fileDescriptor < 0) return;

    float dt = m_elapsedTimer.restart() / 1000.0f;

    // 1. Lecture Accel/Gyro
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);
    char reg = 0x3B;
    char dataAG[14];
    if (write(m_fileDescriptor, &reg, 1) == 1 && read(m_fileDescriptor, dataAG, 14) == 14) {

        // Lecture Accel
        float ax = (int16_t)((dataAG[0] << 8) | dataAG[1]) * (2.0f / 32768.0f);
        float ay = (int16_t)((dataAG[2] << 8) | dataAG[3]) * (2.0f / 32768.0f);
        float az = (int16_t)((dataAG[4] << 8) | dataAG[5]) * (2.0f / 32768.0f);

        // Lecture Gyro ET SOUSTRACTION DU BIAIS
        float gx = ((int16_t)((dataAG[8] << 8) | dataAG[9]) * (250.0f / 32768.0f) * (M_PI / 180.0f)) - m_gyroBias[0];
        float gy = ((int16_t)((dataAG[10] << 8) | dataAG[11]) * (250.0f / 32768.0f) * (M_PI / 180.0f)) - m_gyroBias[1];
        float gz = ((int16_t)((dataAG[12] << 8) | dataAG[13]) * (250.0f / 32768.0f) * (M_PI / 180.0f)) - m_gyroBias[2];

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
                // Application de la calibration Hard/Soft Iron
                float mx = ((int16_t)((dataM[1] << 8) | dataM[0]) - m_magBias[0]) * m_magScale[0];
                float my = ((int16_t)((dataM[3] << 8) | dataM[2]) - m_magBias[1]) * m_magScale[1];
                float mz = ((int16_t)((dataM[5] << 8) | dataM[4]) - m_magBias[2]) * m_magScale[2];

                // 🔴 MODIFICATION CRITIQUE : L'alignement parfait des 9 axes
                // On inverse ax, gy, gz, et on croise my/-mx (Méthode exacte de Kris Winer)
                madgwickUpdate(-ax, ay, az, gx, -gy, -gz, my, -mx, mz, dt);

                // Calcul du Yaw à partir du quaternion
                float yaw = std::atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]);

                // 🔴 MODIFICATION 2 : On retire le signe "moins" (-) devant yaw
                // car le réalignement des axes ci-dessus a déjà corrigé le sens de rotation !
                float heading = yaw * (180.0f / M_PI);

                heading += 2.0f; // Déclinaison magnétique locale (Ajustez selon région)
                if (heading < 0) heading += 360.0f;
                if (heading > 360.0f) heading -= 360.0f;

                qDebug() << "🧭 Boussole -> Cap :" << heading << "° | dt :" << dt;

                if (m_data) m_data->setHeading(static_cast<double>(heading));
            }
        }
    } else {
        qWarning() << "⚠️ Erreur de lecture I2C sur 0x68 (MPU9250 débranché ?)";
    }
}

// --- LE VÉRITABLE FILTRE DE MADGWICK (KRIS WINER) ---
void Mpu9250Source::madgwickUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float dt) {
    float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];
    float norm;
    float hx, hy, _2bx, _2bz;
    float s1, s2, s3, s4;
    float qDot1, qDot2, qDot3, qDot4;

    // Variables auxiliaires pour éviter les calculs répétitifs
    float _2q1mx; float _2q1my; float _2q1mz; float _2q2mx;
    float _4bx; float _4bz;
    float _2q1 = 2.0f * q1; float _2q2 = 2.0f * q2; float _2q3 = 2.0f * q3; float _2q4 = 2.0f * q4;
    float _2q1q3 = 2.0f * q1 * q3; float _2q3q4 = 2.0f * q3 * q4;
    float q1q1 = q1 * q1; float q1q2 = q1 * q2; float q1q3 = q1 * q3; float q1q4 = q1 * q4;
    float q2q2 = q2 * q2; float q2q3 = q2 * q3; float q2q4 = q2 * q4;
    float q3q3 = q3 * q3; float q3q4 = q3 * q4; float q4q4 = q4 * q4;

    // Normaliser l'accéléromètre (Gravité)
    norm = std::sqrt(ax * ax + ay * ay + az * az);
    if (norm == 0.0f) return; // handle NaN
    norm = 1.0f/norm;
    ax *= norm; ay *= norm; az *= norm;

    // Normaliser le magnétomètre (Boussole)
    norm = std::sqrt(mx * mx + my * my + mz * mz);
    if (norm == 0.0f) return; // handle NaN
    norm = 1.0f/norm;
    mx *= norm; my *= norm; mz *= norm;

    // Direction de référence du champ magnétique
    _2q1mx = 2.0f * q1 * mx;
    _2q1my = 2.0f * q1 * my;
    _2q1mz = 2.0f * q1 * mz;
    _2q2mx = 2.0f * q2 * mx;
    hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
    hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + _2q3 * mz * q4 - my * q4q4;
    _2bx = std::sqrt(hx * hx + hy * hy);
    _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + _2q3 * my * q4 - mz * q3q3 + mz * q4q4;
    _4bx = 2.0f * _2bx;
    _4bz = 2.0f * _2bz;

    // Descente de gradient : corrige l'orientation du Gyro avec Accel (Gravité) et Mag (Nord)
    s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    norm = std::sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);    // normalise step magnitude
    norm = 1.0f/norm;
    s1 *= norm; s2 *= norm; s3 *= norm; s4 *= norm;

    // Calcul du taux de changement du quaternion
    qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
    qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
    qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
    qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

    // Intégration pour produire le quaternion final (la position)
    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;
    q4 += qDot4 * dt;
    norm = std::sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);    // normalise quaternion
    norm = 1.0f/norm;

    q[0] = q1 * norm;
    q[1] = q2 * norm;
    q[2] = q3 * norm;
    q[3] = q4 * norm;
}

void Mpu9250Source::stop() {
    m_timer->stop();
    if (m_fileDescriptor >= 0) {
        close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
}
