/**
 * @file mpu9250source.cpp
 * @brief Implémentation du contrôleur I2C pour la centrale inertielle MPU9250.
 * @details Gère les appels système bas niveau (ioctl) sous Linux pour communiquer
 * avec les registres du capteur. Contient la logique d'acquisition, l'application
 * des paramètres de calibration (Hard/Soft iron, biais) et l'algorithme de fusion
 * de capteurs (Filtre de Madgwick) nécessaire au calcul précis de l'orientation (Cap).
 */

#include "mpu9250source.h"
#include "telemetrydata.h"
#include <QDebug>
#include <cmath>

// Les bibliothèques système Linux sont isolées pour que Windows ne plante pas à la compilation
#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#endif

Mpu9250Source::Mpu9250Source(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data), m_fileDescriptor(-1) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Mpu9250Source::readSensor);
}

Mpu9250Source::~Mpu9250Source() {
    stop();
}

void Mpu9250Source::start() {
#ifdef Q_OS_LINUX
    qDebug() << "Tentative d'ouverture du bus I2C-1...";
    m_fileDescriptor = open("/dev/i2c-1", O_RDWR);
    if (m_fileDescriptor < 0) {
        qWarning() << "Échec de l'ouverture du bus I2C.";
        return;
    }

    // Connexion à l'adresse I2C du MPU9250
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);

    // Sortie du mode veille (Wake up)
    char config[2] = {0x6B, 0x00};
    write(m_fileDescriptor, config, 2);

    // Configuration des échelles de mesure
    config[0] = 0x1C; config[1] = 0x00; write(m_fileDescriptor, config, 2); // Accéléromètre : +/- 2g
    config[0] = 0x1B; config[1] = 0x00; write(m_fileDescriptor, config, 2); // Gyroscope : 250 dps (degrés par seconde)

    // Activation du filtre passe-bas matériel (Digital Low Pass Filter - DLPF) à 41Hz
    // Essentiel pour lisser les vibrations mécaniques du véhicule
    config[0] = 0x1A; config[1] = 0x03; write(m_fileDescriptor, config, 2); // Gyroscope DLPF
    config[0] = 0x1D; config[1] = 0x03; write(m_fileDescriptor, config, 2); // Accéléromètre DLPF

    // --- Auto-calibration du gyroscope ---
    // Le gyroscope dérive naturellement. On calcule son erreur initiale au repos.
    qDebug() << "Calibration du gyroscope en cours (maintenir le capteur immobile)...";
    float gyroSum[3] = {0, 0, 0};
    int samples = 400;
    char reg = 0x43; // Registre de départ des données gyroscopiques
    char dataG[6];

    for (int i = 0; i < samples; i++) {
        write(m_fileDescriptor, &reg, 1);
        if (read(m_fileDescriptor, dataG, 6) == 6) {
            // Conversion des valeurs brutes en radians par seconde (rad/s)
            gyroSum[0] += (int16_t)((dataG[0] << 8) | dataG[1]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
            gyroSum[1] += (int16_t)((dataG[2] << 8) | dataG[3]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
            gyroSum[2] += (int16_t)((dataG[4] << 8) | dataG[5]) * (250.0f / 32768.0f) * (M_PI / 180.0f);
        }
        usleep(5000); // Échantillonnage toutes les 5ms (total 2 seconde)
    }
    m_gyroBias[0] = gyroSum[0] / samples;
    m_gyroBias[1] = gyroSum[1] / samples;
    m_gyroBias[2] = gyroSum[2] / samples;
    qDebug() << "Calibration terminée. Biais du gyroscope calculés.";

    // --- Activation du mode Bypass pour le Magnétomètre (AK8963) ---
    // Le magnétomètre a sa propre adresse I2C (0x0C) accessible via le MPU
    config[0] = 0x37; config[1] = 0x02; write(m_fileDescriptor, config, 2);
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);

    // Configuration du magnétomètre : 16 bits de résolution, mode continu (100Hz)
    config[0] = 0x0A; config[1] = 0x16; write(m_fileDescriptor, config, 2);
    qDebug() << "Acquisition MPU9250 démarrée avec succès.";

#else
    qWarning() << "[MODE SIMULATION] - Code compilé sous Windows/Autre. Le matériel I2C est bypassé.";
#endif

    m_elapsedTimer.start();
    m_timer->start(100); // Boucle de lecture à 10Hz
}

void Mpu9250Source::readSensor() {
    // ------------------------------------------------------------------------
    // 0. VÉRIFICATION ET TEMPS ÉCOULÉ
    // ------------------------------------------------------------------------

    // Le filtre de Madgwick a besoin de savoir exactement combien de temps s'est
    // écoulé depuis le dernier calcul pour déduire de combien on a tourné
    // (Vitesse angulaire * temps = Angle).
    // On prend le temps en millisecondes et on divise par 1000 pour l'avoir en secondes (dt).
    float dt = m_elapsedTimer.restart() / 1000.0f;

#ifdef Q_OS_LINUX
    // Si le capteur n'est pas bien connecté ou initialisé, on annule tout.
    if (m_fileDescriptor < 0) return;

    // ------------------------------------------------------------------------
    // 1. LECTURE DE L'ACCÉLÉROMÈTRE ET DU GYROSCOPE
    // ------------------------------------------------------------------------

    // On cible le composant principal (Accel + Gyro) à l'adresse I2C 0x68.
    ioctl(m_fileDescriptor, I2C_SLAVE, 0x68);

    // On demande à lire à partir de la "case mémoire" (registre) 0x3B.
    // C'est là que commencent les données de l'accéléromètre.
    char reg = 0x3B;

    // On va lire 14 octets d'un coup pour gagner du temps :
    // - 6 octets pour l'accéléromètre (X, Y, Z)
    // - 2 octets pour le capteur de température interne
    // - 6 octets pour le gyroscope (X, Y, Z)
    char dataAG[14];

    if (write(m_fileDescriptor, &reg, 1) == 1 && read(m_fileDescriptor, dataAG, 14) == 14) {

        // --- DÉCODAGE DE L'ACCÉLÉROMÈTRE ---
        // Le capteur envoie les données "découpées" en morceaux de 8 bits (1 octet).
        // Mais nos mesures font 16 bits ! Il faut recoller les morceaux.
        // On prend le premier octet (High), on le décale de 8 zéros vers la gauche (<< 8),
        // puis on "fusionne" avec le deuxième octet (Low) grâce à l'opérateur "OU" logique (|).
        // Ensuite, on convertit la valeur brute (-32768 à +32767) en 'g' (gravité).
        // Comme on a configuré le capteur sur +/- 2g max, on divise 2 par 32768.
        float ax = (int16_t)((dataAG[0] << 8) | dataAG[1]) * (2.0f / 32768.0f);
        float ay = (int16_t)((dataAG[2] << 8) | dataAG[3]) * (2.0f / 32768.0f);
        float az = (int16_t)((dataAG[4] << 8) | dataAG[5]) * (2.0f / 32768.0f);

        // --- DÉCODAGE DU GYROSCOPE ---
        // On fait le même "recollage" de bits (octets 8 à 13, car 6 et 7 c'était la température).
        // L'échelle est de 250 dps (degrés par seconde) maximum, donc on multiplie par (250 / 32768).
        // IMPORTANT : Les mathématiques (Madgwick) fonctionnent en Radians, pas en Degrés !
        // On multiplie donc par (Pi / 180) pour faire la conversion en Radians/seconde.
        // Enfin, on soustrait l'erreur (le biais) qu'on a mesurée automatiquement au démarrage.
        float gx = ((int16_t)((dataAG[8]  << 8) | dataAG[9]))  * (250.0f / 32768.0f) * (M_PI / 180.0f) - m_gyroBias[0];
        float gy = ((int16_t)((dataAG[10] << 8) | dataAG[11])) * (250.0f / 32768.0f) * (M_PI / 180.0f) - m_gyroBias[1];
        float gz = ((int16_t)((dataAG[12] << 8) | dataAG[13])) * (250.0f / 32768.0f) * (M_PI / 180.0f) - m_gyroBias[2];

        // ------------------------------------------------------------------------
        // 2. LECTURE DU MAGNÉTOMÈTRE (LA BOUSSOLE)
        // ------------------------------------------------------------------------

        // Le magnétomètre est une puce séparée (AK8963) située à l'adresse 0x0C.
        ioctl(m_fileDescriptor, I2C_SLAVE, 0x0C);

        char st1;
        char regSt1 = 0x02; // Registre d'état du magnétomètre
        write(m_fileDescriptor, &regSt1, 1);

        // On lit l'état. Le bit 0 (st1 & 0x01) nous dit "Data Ready".
        // Le magnétomètre est plus lent (100Hz max), on vérifie donc s'il a une nouvelle info à donner.
        if (read(m_fileDescriptor, &st1, 1) == 1 && (st1 & 0x01)) {

            char regMag = 0x03; // Adresse du début des données magnétiques
            char dataM[7]; // 6 octets pour (X,Y,Z) + 1 octet de statut final obligatoire
            write(m_fileDescriptor, &regMag, 1);

            if (read(m_fileDescriptor, dataM, 7) == 7) {
                // ATTENTION PIÈGE : Contrairement à l'accéléromètre, le magnétomètre envoie
                // le "Low byte" (petits bits) AVANT le "High byte" (gros bits).
                // On inverse donc l'ordre dans le calcul (dataM[1] << 8 | dataM[0]).

                // On applique ensuite la calibration :
                // - Hard Iron (soustraction du biais) : annule les champs magnétiques du châssis.
                // - Soft Iron (multiplication par l'échelle) : redonne une forme parfaitement ronde au signal.
                float mx = ((int16_t)((dataM[1] << 8) | dataM[0]) - m_magBias[0]) * m_magScale[0];
                float my = ((int16_t)((dataM[3] << 8) | dataM[2]) - m_magBias[1]) * m_magScale[1];
                float mz = ((int16_t)((dataM[5] << 8) | dataM[4]) - m_magBias[2]) * m_magScale[2];

                // ------------------------------------------------------------------------
                // 3. FUSION DE DONNÉES (FILTRE DE MADGWICK)
                // ------------------------------------------------------------------------

                // Les axes physiques de la puce magnétomètre ne sont pas alignés dans le même
                // sens que ceux du gyroscope/accéléromètre sur la carte électronique.
                // On doit les réaligner pour utiliser le standard NED (Nord-Est-Bas).
                // C'est pour ça qu'on croise des axes (my à la place de mx) et qu'on inverse des signes.
                madgwickUpdate(-ax, ay, az, gx, -gy, -gz, my, -mx, mz, dt);

                // ------------------------------------------------------------------------
                // 4. CALCUL DU CAP COMPENSÉ EN INCLINAISON (TILT-COMPENSATED YAW)
                // ------------------------------------------------------------------------

                // Extraction du Roulis (Roll) et Tangage (Pitch) depuis le Quaternion.
                // Cela nous dit comment la voiture est penchée par rapport à la gravité terrestre.
                float roll = std::atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]));
                float pitch = std::asin(2.0f * (q[0] * q[2] - q[3] * q[1]));

                // On projette les vecteurs magnétiques 'my' et '-mx' sur le plan horizontal (sol)
                // en utilisant le roulis et le tangage. Cela garantit que la boussole pointe
                // toujours le Vrai Nord, même si la voiture monte une pente raide.
                float mag_x_horiz = my * std::cos(pitch) + (-mx) * std::sin(roll) * std::sin(pitch) - mz * std::cos(roll) * std::sin(pitch);
                float mag_y_horiz = (-mx) * std::cos(roll) + mz * std::sin(roll);

                // On calcule l'angle par rapport au Nord Magnétique à partir de cette projection
                float yaw = std::atan2(mag_y_horiz, mag_x_horiz);

                // Conversion des Radians vers les Degrés.
                // On ajoute un signe MOINS (-) car les maths tournent en sens anti-horaire,
                // mais la carte Qt tourne dans le sens horaire (comme une vraie boussole).
                float heading = -yaw * (180.0f / M_PI);

                // Ajout de la Déclinaison Magnétique pour pointer vers le Nord Géographique (Vrai Nord).
                // (2.0° correspond environ à la France actuelle, ajustez si besoin).
                heading += 2.0f;

                // On s'assure que l'angle reste un cercle parfait de 0 à 360°.
                if (heading < 0) heading += 360.0f;
                if (heading >= 360.0f) heading -= 360.0f;

                // ------------------------------------------------------------------------
                // 5. FILTRE DE LISSAGE (PASSE-BAS VISUEL)
                // ------------------------------------------------------------------------

                // On mélange 90% de l'ancien cap avec 10% du nouveau.
                // Cela empêche la carte de faire des micro-tremblements à l'écran à cause des vibrations.
                static float smoothedHeading = heading;

                // Gestion du passage difficile entre 359° et 0° pour éviter que la carte ne fasse
                // un tour complet sur elle-même lors du passage du Nord.
                float diff = heading - smoothedHeading;
                if (diff > 180.0f) smoothedHeading += 360.0f;
                else if (diff < -180.0f) smoothedHeading -= 360.0f;

                smoothedHeading = (smoothedHeading * 0.90f) + (heading * 0.10f);

                // On remet la valeur lissée proprement entre 0 et 360°.
                if (smoothedHeading >= 360.0f) smoothedHeading -= 360.0f;
                else if (smoothedHeading < 0.0f) smoothedHeading += 360.0f;

                // Affichage console pour diagnostiquer
                qDebug() << " Cap :" << smoothedHeading << "° | dt:" << dt;

                // On envoie cet angle tout propre à l'interface graphique (QML) pour faire tourner la carte !
                if (m_data) m_data->setHeading(static_cast<double>(smoothedHeading));
            }
        }
    } else {
        qDebug() << "Erreur de lecture I2C sur l'adresse 0x68 (Capteur débranché ou indisponible).";
    }
#else
    // Empêche le compilateur Windows d'afficher un avertissement pour la variable non utilisée
    Q_UNUSED(dt);
#endif
}

/**
 * @brief Filtre de fusion de capteurs de Madgwick (Implémentation optimisée).
 * @details Cet algorithme estime l'orientation 3D (quaternions) en fusionnant les données :
 * - Le Gyroscope calcule les rotations rapides (mais dérive dans le temps).
 * - L'Accéléromètre trouve la gravité (vecteur Bas) pour corriger le tangage/roulis.
 * - Le Magnétomètre trouve le Nord magnétique pour corriger le lacet (Cap).
 * La descente de gradient permet de converger vers l'orientation réelle.
 */
void Mpu9250Source::madgwickUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float dt) {
    float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];
    float norm;
    float hx, hy, _2bx, _2bz;
    float s1, s2, s3, s4;
    float qDot1, qDot2, qDot3, qDot4;

    // Variables auxiliaires pour optimiser les performances (éviter les calculs répétitifs)
    float _2q1mx; float _2q1my; float _2q1mz; float _2q2mx;
    float _4bx; float _4bz;
    float _2q1 = 2.0f * q1; float _2q2 = 2.0f * q2; float _2q3 = 2.0f * q3; float _2q4 = 2.0f * q4;
    float _2q1q3 = 2.0f * q1 * q3; float _2q3q4 = 2.0f * q3 * q4;
    float q1q1 = q1 * q1; float q1q2 = q1 * q2; float q1q3 = q1 * q3; float q1q4 = q1 * q4;
    float q2q2 = q2 * q2; float q2q3 = q2 * q3; float q2q4 = q2 * q4;
    float q3q3 = q3 * q3; float q3q4 = q3 * q4; float q4q4 = q4 * q4;

    // 1. Normalisation du vecteur d'accélération (Gravité)
    norm = std::sqrt(ax * ax + ay * ay + az * az);
    if (norm == 0.0f) return; // Sécurité contre la division par zéro
    norm = 1.0f/norm;
    ax *= norm; ay *= norm; az *= norm;

    // 2. Normalisation du vecteur magnétique (Nord)
    norm = std::sqrt(mx * mx + my * my + mz * mz);
    if (norm == 0.0f) return;
    norm = 1.0f/norm;
    mx *= norm; my *= norm; mz *= norm;

    // 3. Calcul de la direction de référence du champ magnétique
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

    // 4. Descente de gradient : Calcul de l'erreur d'orientation
    s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    norm = std::sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4); // Normalisation de l'erreur
    norm = 1.0f/norm;
    s1 *= norm; s2 *= norm; s3 *= norm; s4 *= norm;

    // 5. Calcul du taux de changement du quaternion (Fusion Gyroscope + Erreur corrigée par beta)
    qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
    qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
    qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
    qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

    // 6. Intégration dans le temps pour obtenir le nouveau quaternion d'orientation
    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;
    q4 += qDot4 * dt;

    // 7. Normalisation finale du quaternion
    norm = std::sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
    norm = 1.0f/norm;

    q[0] = q1 * norm;
    q[1] = q2 * norm;
    q[2] = q3 * norm;
    q[3] = q4 * norm;
}

void Mpu9250Source::stop() {
    m_timer->stop();
#ifdef Q_OS_LINUX
    if (m_fileDescriptor >= 0) {
        close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
#endif
}
