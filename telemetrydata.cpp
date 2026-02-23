// Rôle architectural: implémentation du modèle de données télémétriques.
// Responsabilités: appliquer la validation minimale des valeurs et émettre les notifications de changement.
// Dépendances principales: QtGlobal/QtMath et contrat de propriétés de TelemetryData.

#include "telemetrydata.h"
#include <QtGlobal>
#include <QtMath>

TelemetryData::TelemetryData(QObject* parent) : QObject(parent) {}

void TelemetryData::setSpeedKmh(double v){
    if (qFuzzyCompare(m_speedKmh, v)) return;
    m_speedKmh=v; emit speedKmhChanged();
}
void TelemetryData::setGpsOk(bool v){
    if (m_gpsOk==v) return;
    m_gpsOk=v; emit gpsOkChanged();
}
void TelemetryData::setLat(double v){
    if (qFuzzyCompare(m_lat, v)) return;
    m_lat=v; emit latChanged();
}
void TelemetryData::setLon(double v){
    if (qFuzzyCompare(m_lon, v)) return;
    m_lon=v; emit lonChanged();
}
void TelemetryData::setAlertText(const QString& v){
    if (m_alertText==v) return;
    m_alertText=v; emit alertTextChanged();
}
void TelemetryData::setAlertLevel(int v){
    if (m_alertLevel==v) return;
    m_alertLevel=v; emit alertLevelChanged();
}
void TelemetryData::setHeading(double v){
    if (qFuzzyCompare(m_heading, v)) return;
    m_heading = v;
    emit headingChanged();
}
