/**
 * @file gpstelemetrysource.cpp
 * @brief Impl�mentation de la source GPS mat�rielle.
 * @details Responsabilit�s : Configurer le port s�rie, d�coder le flux NMEA en continu
 * et traduire les mesures brutes en t�l�m�trie exploitable par l'interface graphique.
 * D�pendances principales : Qt SerialPort, Qt Positioning et TelemetryData.
 */

#include "gpstelemetrysource.h"
#include "telemetrydata.h"
#include <QDebug>

GpsTelemetrySource::GpsTelemetrySource(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data)
{
    // Initialisation de l'interface s�rie mat�rielle
    m_serial = new QSerialPort(this);
}

GpsTelemetrySource::~GpsTelemetrySource() {
    stop();
}

void GpsTelemetrySource::start(const QString& portName) {
    // Red�marrage idempotent : on repart d'un �tat propre et on referme le port
    // s'il �tait d�j� ouvert avant toute nouvelle tentative.
    stop();

    // Configuration de la connexion physique au module GPS (ex: NEO-6M)
    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud9600); // 9600 bauds est le standard industriel NMEA par d�faut

    if (!m_serial->open(QIODevice::ReadOnly)) {
        qCritical() << "? Erreur : Impossible d'ouvrir le module GPS sur le port" << portName;
        if(m_data) m_data->setGpsOk(false);
        return;
    }

    // Cr�ation du parseur NMEA en "RealTimeMode" (lit le flux en direct au lieu d'un fichier log)
    m_nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
    m_nmeaSource->setDevice(m_serial);

    // Connexion du moteur Qt Positioning � notre logique m�tier
    connect(m_nmeaSource, &QNmeaPositionInfoSource::positionUpdated,
            this, &GpsTelemetrySource::onPositionUpdated);

    // D�marrage de la boucle de lecture
    m_nmeaSource->startUpdates();

    qDebug() << "? GPS D�marr� (Mode Qt Positioning) sur" << portName;
}

void GpsTelemetrySource::stop() {
    // L'arr�t explicite du parseur et la suppression de l'objet �vitent
    // des callbacks fant�mes lors des changements d'�tat de l'application.
    if (m_nmeaSource) {
        m_nmeaSource->stopUpdates();
        delete m_nmeaSource;
        m_nmeaSource = nullptr;
    }

    // Lib�ration mat�rielle du port s�rie
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

void GpsTelemetrySource::onPositionUpdated(const QGeoPositionInfo &info) {
    if (!m_data) return;

    if (info.isValid()) {
        // Le module GPS "fixe" les satellites (position 3D valid�e)
        m_data->setGpsOk(true);

        QGeoCoordinate coord = info.coordinate();
        m_data->setLat(coord.latitude());
        m_data->setLon(coord.longitude());

        // Extraction de la vitesse (si la trame NMEA RMC ou VTG la fournit)
        double speedMs = 0.0;
        if (info.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
            speedMs = info.attribute(QGeoPositionInfo::GroundSpeed); // En m�tres par seconde
            m_data->setSpeedKmh(speedMs * 3.6); // Conversion en km/h pour l'affichage tableau de bord
        }

        // Extraction du cap (Direction)
        if (info.hasAttribute(QGeoPositionInfo::Direction)) {
            double course = info.attribute(QGeoPositionInfo::Direction);

            // LOGIQUE M�TIER CRITIQUE :
            // Sous une faible vitesse, le calcul de cap (Heading) par le GPS devient erratique
            // car le module ne peut plus d�terminer l'avant de l'arri�re.
            // On applique un seuil (3 km/h) pour �viter que la carte GPS ne pivote brutalement
            // dans tous les sens lorsque le v�hicule est arr�t� � un feu rouge.
            /*if (speedMs * 3.6 > 3.0) {
                m_data->setHeading(course);
            }*/
        }
    } else {
        // Le GPS est allum� mais cherche encore ses satellites (Cold/Warm start)
        m_data->setGpsOk(false);
        qDebug() << "GPS : En attente de satellites (No Fix)...";
    }
}
