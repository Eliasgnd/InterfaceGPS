// Rôle architectural: implémentation de la source GPS matérielle.
// Responsabilités: configurer le port série, décoder le NMEA en continu et traduire les mesures en télémétrie exploitable.
// Dépendances principales: Qt SerialPort, Qt Positioning et TelemetryData.

#include "gpstelemetrysource.h"
#include "telemetrydata.h"
#include <QDebug>

GpsTelemetrySource::GpsTelemetrySource(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data)
{

    m_serial = new QSerialPort(this);
}

GpsTelemetrySource::~GpsTelemetrySource() {
    stop();
}

void GpsTelemetrySource::start(const QString& portName) {
    // Redémarrage idempotent: on repart d'un état propre avant toute nouvelle ouverture série.
    stop();


    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud9600);



    if (!m_serial->open(QIODevice::ReadOnly)) {
        qCritical() << "? Erreur : Impossible d'ouvrir le GPS sur" << portName;
        if(m_data) m_data->setGpsOk(false);
        return;
    }



    m_nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);


    m_nmeaSource->setDevice(m_serial);


    connect(m_nmeaSource, &QNmeaPositionInfoSource::positionUpdated,
            this, &GpsTelemetrySource::onPositionUpdated);


    m_nmeaSource->startUpdates();

    qDebug() << "? GPS D�marr� (Mode Qt Positioning) sur" << portName;
}

void GpsTelemetrySource::stop() {
    // L'arrêt explicite évite des callbacks tardifs lors des changements de page ou de shutdown.
    if (m_nmeaSource) {
        m_nmeaSource->stopUpdates();
        delete m_nmeaSource;
        m_nmeaSource = nullptr;
    }
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

void GpsTelemetrySource::onPositionUpdated(const QGeoPositionInfo &info) {
    if (!m_data) return;

    if (info.isValid()) {
        m_data->setGpsOk(true);

        QGeoCoordinate coord = info.coordinate();
        m_data->setLat(coord.latitude());
        m_data->setLon(coord.longitude());


        double speedMs = 0.0;
        if (info.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
            speedMs = info.attribute(QGeoPositionInfo::GroundSpeed);
            m_data->setSpeedKmh(speedMs * 3.6);
        }


        // Sous faible vitesse, le cap GPS est instable; un seuil évite les rotations erratiques en UI.
        if (info.hasAttribute(QGeoPositionInfo::Direction)) {
            double course = info.attribute(QGeoPositionInfo::Direction);




            if (speedMs * 3.6 > 3.0) {
                m_data->setHeading(course);
            }
        } else {

        }

    } else {
        m_data->setGpsOk(false);
        qDebug() << "GPS : En attente de satellites (No Fix)...";
    }
}
