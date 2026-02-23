/**
 * @file telemetrydata.cpp
 * @brief Implémentation du modèle de données télémétriques.
 * @details Applique une validation minimale des valeurs avant de les assigner
 * et émet les signaux de changement uniquement si la valeur a réellement évolué
 * (pour éviter de surcharger l'interface graphique de mises à jour inutiles).
 */

#include "telemetrydata.h"
#include <QtGlobal>
#include <QtMath>

TelemetryData::TelemetryData(QObject* parent) : QObject(parent)
{
    // L'initialisation se fait via les valeurs par défaut dans le fichier d'en-tête (.h)
}

void TelemetryData::setSpeedKmh(double v) {
    // qFuzzyCompare est utilisé pour comparer des nombres à virgule flottante (double)
    // afin d'éviter des faux positifs liés aux imprécisions mathématiques de l'ordinateur.
    if (qFuzzyCompare(m_speedKmh, v)) return;

    m_speedKmh = v;
    emit speedKmhChanged(); // Notifie l'UI (QML/C++) de la nouvelle valeur
}

void TelemetryData::setGpsOk(bool v) {
    // Si la valeur est identique à l'actuelle, on ne fait rien
    if (m_gpsOk == v) return;

    m_gpsOk = v;
    emit gpsOkChanged();
}

void TelemetryData::setLat(double v) {
    if (qFuzzyCompare(m_lat, v)) return;

    m_lat = v;
    emit latChanged();
}

void TelemetryData::setLon(double v) {
    if (qFuzzyCompare(m_lon, v)) return;

    m_lon = v;
    emit lonChanged();
}

void TelemetryData::setHeading(double v) {
    if (qFuzzyCompare(m_heading, v)) return;

    m_heading = v;
    emit headingChanged();
}

void TelemetryData::setAlertText(const QString& v) {
    if (m_alertText == v) return;

    m_alertText = v;
    emit alertTextChanged();
}

void TelemetryData::setAlertLevel(int v) {
    if (m_alertLevel == v) return;

    m_alertLevel = v;
    emit alertLevelChanged();
}
