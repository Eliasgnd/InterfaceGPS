/**
 * @file mpu9250source.h
 * @brief Rôle architectural : Source de données inertielles et boussole matérielle (I2C).
 */

#ifndef MPU9250SOURCE_H
#define MPU9250SOURCE_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

class TelemetryData;

/**
 * @class Mpu9250Source
 * @brief Contrôleur matériel d'acquisition pour la centrale inertielle MPU9250.
 * @details Communique via le bus I2C physique (ex: broches du Raspberry Pi) pour
 * récupérer les données brutes de l'accéléromètre, du gyroscope et du magnétomètre.
 * Utilise ensuite l'algorithme de fusion de capteurs de Madgwick pour calculer
 * l'orientation 3D (quaternions) et en déduire le cap (Heading) du véhicule.
 */
class Mpu9250Source : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructeur de la source MPU9250.
     * @param data Pointeur vers le modèle de télémétrie partagé pour y injecter le cap.
     * @param parent Objet parent pour la gestion automatique de la mémoire.
     */
    explicit Mpu9250Source(TelemetryData* data, QObject* parent = nullptr);

    /**
     * @brief Destructeur. Assure la fermeture propre du descripteur de fichier I2C.
     */
    ~Mpu9250Source();

    /**
     * @brief Démarre l'acquisition matérielle.
     * @details Ouvre le bus I2C, réveille le capteur, configure les filtres passe-bas (DLPF),
     * exécute une auto-calibration du gyroscope (stationnaire), et lance le timer de lecture.
     */
    void start();

    /**
     * @brief Arrête l'acquisition et ferme la connexion I2C.
     */
    void stop();

private slots:
    /**
     * @brief Routine de lecture appelée à intervalle régulier par le timer.
     * @details Lit les 9 axes du capteur, applique les calibrations (Hard/Soft Iron, biais gyro),
     * exécute le filtre de Madgwick et met à jour le cap dans TelemetryData.
     */
    void readSensor();

private:
    /**
     * @brief Implémentation du filtre de fusion de capteurs de Madgwick (Méthode Kris Winer).
     * @param ax Accélération sur l'axe X (en g).
     * @param ay Accélération sur l'axe Y (en g).
     * @param az Accélération sur l'axe Z (en g).
     * @param gx Vitesse angulaire sur l'axe X (en rad/s).
     * @param gy Vitesse angulaire sur l'axe Y (en rad/s).
     * @param gz Vitesse angulaire sur l'axe Z (en rad/s).
     * @param mx Champ magnétique sur l'axe X (normalisé).
     * @param my Champ magnétique sur l'axe Y (normalisé).
     * @param mz Champ magnétique sur l'axe Z (normalisé).
     * @param dt Delta de temps écoulé depuis la dernière mise à jour (en secondes).
     */
    void madgwickUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float dt);

    // --- Paramètres matériels ---
    TelemetryData* m_data;          ///< Pointeur vers les données partagées de l'application
    QTimer* m_timer;                ///< Timer cadençant la lecture I2C
    QElapsedTimer m_elapsedTimer;   ///< Timer haute précision pour calculer le dt de Madgwick
    int m_fileDescriptor;           ///< Descripteur du bus I2C système Linux

    // --- État du filtre ---
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f}; ///< Quaternions représentant l'orientation actuelle
    const float beta = 0.1f;               ///< Gain du filtre de Madgwick (équilibre entre gyro et accéléromètre)

    // --- Paramètres de calibration ---
    float m_magBias[3] = {108.0f, 144.0f, -77.0f};          ///< Biais magnétomètre (Hard Iron)
    float m_magScale[3] = {0.991251f, 1.03755f, 0.973368f}; ///< Échelle magnétomètre (Soft Iron)
    float m_gyroBias[3] = {0.0f, 0.0f, 0.0f};               ///< Biais gyroscope (calculé dynamiquement au start)
};

#endif // MPU9250SOURCE_H
