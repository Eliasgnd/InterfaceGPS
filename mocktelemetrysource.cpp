#include "mocktelemetrysource.h"
#include "telemetrydata.h"
#include <QGeoCoordinate>
#include <QDebug>

MockTelemetrySource::MockTelemetrySource(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data)
{
    // Position fixe : IUT de Troyes (ou centre ville)
    // Vous pouvez changer ces coordonnées pour tester d'autres départs
    m_currentPos = QGeoCoordinate(48.2715, 4.0645);

    // Timer plus lent, juste pour simuler que le GPS est "vivant"
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &MockTelemetrySource::tick);
}

void MockTelemetrySource::start(){ m_timer.start(); }
void MockTelemetrySource::stop(){ m_timer.stop(); }

void MockTelemetrySource::tick(){
    if(!m_data) return;

    // ON NE BOUGE PLUS : On renvoie toujours la même position
    m_data->setLat(m_currentPos.latitude());
    m_data->setLon(m_currentPos.longitude());

    // Vitesse à 0
    m_data->setSpeedKmh(0.0);

    // Cap vers le Nord (0) ou l'Est (90)
    // Assurez-vous d'avoir ajouté setHeading dans TelemetryData si vous l'utilisez
    // m_data->setHeading(0.0);

    // Batterie fixe ou qui baisse doucement
    m_data->setBatteryPercent(95);
    m_data->setGpsOk(true);
}
