/**
 * @file gpstelemetrysource.h
 * @brief Rôle architectural : Source de télémétrie GPS branchée sur un flux NMEA série.
 * @details Responsabilités : Démarrer/arrêter la lecture sur le port série matériel
 * et publier les mises à jour de position (latitude, longitude, vitesse, cap) vers le bus TelemetryData.
 * Dépendances principales : QSerialPort, QNmeaPositionInfoSource et QGeoPositionInfo.
 */

#pragma once
#include <QObject>
#include <QSerialPort>
#include <QNmeaPositionInfoSource>
#include <QGeoPositionInfo>

class TelemetryData;

/**
 * @class GpsTelemetrySource
 * @brief Contrôleur matériel d'acquisition GPS.
 * * Écoute un port série physique (ex: GPIO du Raspberry Pi ou USB).
 * * Utilise le moteur Qt Positioning pour parser les trames NMEA standard (GPGGA, GPRMC, etc.).
 * * Filtre et transmet les données propres au modèle de télémétrie global de l'application.
 */
class GpsTelemetrySource : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructeur de la source GPS.
     * @param data Pointeur vers le modèle de télémétrie partagé à mettre à jour.
     * @param parent Objet parent pour la gestion mémoire.
     */
    explicit GpsTelemetrySource(TelemetryData* data, QObject* parent = nullptr);

    /**
     * @brief Destructeur. Assure la fermeture propre du port série.
     */
    ~GpsTelemetrySource();

    /**
     * @brief Démarre l'acquisition des données GPS.
     * Configure le port série (baudrate) et lance le parsing NMEA en temps réel.
     * @param portName Le nom du port matériel (ex: "/dev/serial0" sur RPi, ou "COM3" sur Windows).
     */
    void start(const QString& portName = "/dev/serial0");

    /**
     * @brief Arrête l'acquisition GPS et libère le port matériel.
     */
    void stop();

private slots:
    /**
     * @brief Slot déclenché automatiquement par Qt chaque fois qu'une trame GPS valide est décodée.
     * Extrait les coordonnées, la vitesse et le cap, puis les injecte dans TelemetryData.
     * @param info Objet Qt contenant toutes les métriques de position calculées.
     */
    void onPositionUpdated(const QGeoPositionInfo &info);

private:
    // --- ATTRIBUTS ---
    TelemetryData* m_data = nullptr;                ///< Référence au modèle de données partagé.
    QSerialPort* m_serial = nullptr;                ///< Interface matérielle série UART/USB.
    QNmeaPositionInfoSource* m_nmeaSource = nullptr; ///< Parseur de trames NMEA intégré à Qt.
};
