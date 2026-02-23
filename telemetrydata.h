/**
 * @file telemetrydata.h
 * @brief Rôle architectural : Modèle central de télémétrie partagé entre les modules C++ et l'interface QML.
 * @details Responsabilités : Stocker les mesures courantes du véhicule (vitesse, position, alertes)
 * et notifier l'interface graphique de tout changement via le système de propriétés de Qt.
 * Dépendances principales : QObject, système de Q_PROPERTY de Qt.
 */

#pragma once
#include <QObject>
#include <QVariantList>

/**
 * @class TelemetryData
 * @brief Classe représentant les données en temps réel du véhicule.
 * * Cette classe hérite de QObject pour permettre une intégration fluide avec QML.
 * Chaque donnée (vitesse, position, etc.) est exposée sous forme de Q_PROPERTY,
 * ce qui permet à l'interface graphique de se mettre à jour automatiquement
 * dès qu'une valeur change (grâce aux signaux NOTIFY).
 */
class TelemetryData : public QObject {
    Q_OBJECT

    // --- PROPRIÉTÉS EXPOSÉES À L'INTERFACE (QML) ---
    Q_PROPERTY(double speedKmh READ speedKmh WRITE setSpeedKmh NOTIFY speedKmhChanged)
    Q_PROPERTY(bool gpsOk READ gpsOk WRITE setGpsOk NOTIFY gpsOkChanged)
    Q_PROPERTY(double lat READ lat WRITE setLat NOTIFY latChanged)
    Q_PROPERTY(double lon READ lon WRITE setLon NOTIFY lonChanged)
    Q_PROPERTY(double heading READ heading WRITE setHeading NOTIFY headingChanged)
    Q_PROPERTY(QString alertText READ alertText WRITE setAlertText NOTIFY alertTextChanged)
    Q_PROPERTY(int alertLevel READ alertLevel WRITE setAlertLevel NOTIFY alertLevelChanged)

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
    QString alertText() const { return m_alertText; }///< Retourne le texte de l'alerte en cours.
    int alertLevel() const { return m_alertLevel; }  ///< Retourne le niveau de criticité de l'alerte (0 = aucune, 1 = avertissement, 2 = critique).

public slots:
    // --- SETTERS (Modificateurs) ---
    // Ces méthodes peuvent être appelées dynamiquement, y compris depuis QML.

    void setSpeedKmh(double v);
    void setGpsOk(bool v);
    void setLat(double v);
    void setLon(double v);
    void setHeading(double v);
    void setAlertText(const QString& v);
    void setAlertLevel(int v);

signals:
    // --- SIGNAUX DE NOTIFICATION ---
    // Émis automatiquement lorsque la valeur d'une propriété correspondante est modifiée.

    void speedKmhChanged();
    void gpsOkChanged();
    void latChanged();
    void lonChanged();
    void headingChanged();
    void alertTextChanged();
    void alertLevelChanged();

private:
    // --- VARIABLES INTERNES ---
    double m_speedKmh = 0.0;           ///< Vitesse en km/h
    bool m_gpsOk = true;               ///< État de la connexion GPS
    double m_lat = 48.8566;            ///< Latitude (Par défaut: Paris)
    double m_lon = 2.3522;             ///< Longitude (Par défaut: Paris)
    double m_heading = 0.0;            ///< Cap en degrés (0 à 360)
    QString m_alertText;               ///< Message d'alerte courant
    int m_alertLevel = 0;              ///< Niveau d'alerte (0, 1 ou 2)
};
