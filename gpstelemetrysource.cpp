#include "gpstelemetrysource.h"
#include "telemetrydata.h"
#include <QDebug>

GpsTelemetrySource::GpsTelemetrySource(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data)
{
    // On pr�pare le port s�rie
    m_serial = new QSerialPort(this);
}

GpsTelemetrySource::~GpsTelemetrySource() {
    stop();
}

void GpsTelemetrySource::start(const QString& portName) {
    stop(); // On arr�te tout avant de red�marrer pour �tre propre

    // 1. Configuration du Port S�rie (Hardware)
    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud9600);
    // Le reste (Data8, NoParity, OneStop) est la configuration par d�faut,
    // mais on peut le pr�ciser si n�cessaire.

    if (!m_serial->open(QIODevice::ReadOnly)) {
        qCritical() << "? Erreur : Impossible d'ouvrir le GPS sur" << portName;
        if(m_data) m_data->setGpsOk(false);
        return;
    }

    // 2. Cr�ation et configuration du moteur NMEA de Qt
    // "RealTimeMode" signifie qu'il lit le flux au fur et � mesure
    m_nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);

    // On lui donne le port s�rie ouvert
    m_nmeaSource->setDevice(m_serial);

    // 3. Connexion du signal de mise � jour
    connect(m_nmeaSource, &QNmeaPositionInfoSource::positionUpdated,
            this, &GpsTelemetrySource::onPositionUpdated);

    // 4. D�marrage du d�codage
    m_nmeaSource->startUpdates();

    qDebug() << "? GPS D�marr� (Mode Qt Positioning) sur" << portName;
}

void GpsTelemetrySource::stop() {
    if (m_nmeaSource) {
        m_nmeaSource->stopUpdates();
        delete m_nmeaSource;
        m_nmeaSource = nullptr;
    }
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

// C'est ici que la magie op�re : Qt a d�j� d�cod� le texte pour nous !
void GpsTelemetrySource::onPositionUpdated(const QGeoPositionInfo &info) {
    if (!m_data) return;

    if (info.isValid()) {
        m_data->setGpsOk(true);

        QGeoCoordinate coord = info.coordinate();
        m_data->setLat(coord.latitude());
        m_data->setLon(coord.longitude());

        // Récupération vitesse (m/s -> km/h)
        double speedMs = 0.0;
        if (info.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
            speedMs = info.attribute(QGeoPositionInfo::GroundSpeed);
            m_data->setSpeedKmh(speedMs * 3.6);
        }

        // --- GESTION ET VÉRIFICATION DE L'ORIENTATION ---
        if (info.hasAttribute(QGeoPositionInfo::Direction)) {
            double course = info.attribute(QGeoPositionInfo::Direction);

            // LOG DE DÉBUG : Regardez la console "Application Output" dans Qt Creator
            qDebug() << "GPS Fix -> Lat:" << coord.latitude()
                     << "Lon:" << coord.longitude()
                     << "Vitesse:" << (speedMs*3.6) << "km/h"
                     << "Cap (Raw):" << course;

            if (speedMs * 3.6 > 3.0) {
                m_data->setHeading(course);
            }
        } else {
            qDebug() << "GPS Fix mais PAS d'info de direction (Cap)";
        }

    } else {
        m_data->setGpsOk(false);
        qDebug() << "GPS : En attente de satellites (No Fix)...";
    }
}
