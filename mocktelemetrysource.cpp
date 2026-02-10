#include "mocktelemetrysource.h"
#include "telemetrydata.h"
#include <QtMath>
#include <QRandomGenerator>

MockTelemetrySource::MockTelemetrySource(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data)
{
    m_timer.setInterval(100); // 10Hz
    connect(&m_timer, &QTimer::timeout, this, &MockTelemetrySource::tick);
}

void MockTelemetrySource::start(){ m_timer.start(); }
void MockTelemetrySource::stop(){ m_timer.stop(); }

void MockTelemetrySource::tick(){
    if(!m_data) return;
    m_t += 0.1;

    double speed = 25.0 + 15.0 * qSin(m_t*0.7);
    if(speed < 0) speed = 0;

    if(QRandomGenerator::global()->bounded(20)==0)
        m_batt = qMax(0, m_batt-1);

    bool rev = m_data->reverse();
    if(QRandomGenerator::global()->bounded(120)==0) rev = !rev;

    double lat = m_data->lat() + 0.00001 * qCos(m_t);
    double lon = m_data->lon() + 0.00001 * qSin(m_t);

    int level=0; QString text;
    if(m_batt <= 20){ level=1; text="Batterie faible"; }
    if(QRandomGenerator::global()->bounded(400)==0){ level=2; text="Surchauffe controleur"; }

    m_data->setSpeedKmh(speed);
    m_data->setBatteryPercent(m_batt);
    m_data->setReverse(rev);
    m_data->setGpsOk(true);
    m_data->setLat(lat);
    m_data->setLon(lon);
    m_data->setAlertLevel(level);
    m_data->setAlertText(text);
}
