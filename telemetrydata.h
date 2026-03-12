/**
 * @file telemetrydata.h
 * @brief Rôle architectural : Modèle central de télémétrie partagé entre les modules C++ et l'interface QML.
 * @details Responsabilités : Stocker les mesures courantes du véhicule (vitesse, position, alertes)
 * et notifier l'interface graphique de tout changement via des signaux Qt dédiés.
 * Dépendances principales : QObject et mécanisme signals/slots.
 */

#pragma once
#include <QObject>
#include <QVariantList>

/**
 * @class TelemetryData
 * @brief Classe représentant les données en temps réel du véhicule.
 * Cette classe hérite de QObject et centralise les mesures runtime du véhicule.
 * Les modules producteurs (GPS, IMU, etc.) écrivent via les setters,
 * tandis que les consommateurs UI réagissent aux signaux de changement associés.
 */
class TelemetryData : public QObject {
    Q_OBJECT

    // --- ÉTAT MÉTIER EXPOSÉ VIA API C++/SIGNALS ---
    // Note: les propriétés Q_PROPERTY sont volontairement désactivées pour l'instant.
    // L'interface actuelle consomme ces données via liaisons C++ directes
    // (QObject + signaux), ce qui évite de dépendre d'un binding QML global.

public:
    /**
     * @brief Constructeur par défaut de TelemetryData.
     * @param parent Objet parent pour la gestion automatique de la mémoire (QObject tree).
     */
    explicit TelemetryData(QObject* parent = nullptr);

    // --- GETTERS (Accesseurs) ---
    double speedKmh() const { return m_speedKmh; }   ///< Retourne la vitesse actuelle en km/h.
    bool gpsOk() const { return m_gpsOk; }           ///< Retourne true si le signal GPS est valide.
    double lat() const { return m_lat; }             ///< Retourne la latitude actuelle en degrés.
    double lon() const { return m_lon; }             ///< Retourne la longitude actuelle en degrés.
    double heading() const { return m_heading; }     ///< Retourne le cap actuel du véhicule en degrés (0 = Nord).

public slots:
    // --- SETTERS (Modificateurs) ---
    // Ces méthodes peuvent être appelées dynamiquement, y compris depuis QML.

    void setSpeedKmh(double v);
    void setGpsOk(bool v);
    void setLat(double v);
    void setLon(double v);
    void setHeading(double v);

signals:
    // --- SIGNAUX DE NOTIFICATION ---
    // Émis uniquement en cas de changement effectif de valeur.

    /** @brief Notifie une mise à jour de la vitesse véhicule (km/h). */
    void speedKmhChanged();

    /** @brief Notifie un changement d'état de validité GPS (fix disponible ou non). */
    void gpsOkChanged();

    /** @brief Notifie une mise à jour de latitude. */
    void latChanged();

    /** @brief Notifie une mise à jour de longitude. */
    void lonChanged();

    /** @brief Notifie une mise à jour de cap/heading. */
    void headingChanged();

private:
    // --- VARIABLES INTERNES ---
    double m_speedKmh = 0.0;           ///< Vitesse en km/h
    bool m_gpsOk = true;               ///< État de la connexion GPS
    double m_lat = 48.8566;            ///< Latitude (Par défaut: Paris)
    double m_lon = 2.3522;             ///< Longitude (Par défaut: Paris)
    double m_heading = 0.0;            ///< Cap en degrés (0 à 360)
};
