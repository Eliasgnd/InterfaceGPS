#include "mocktelemetrysource.h"
#include "telemetrydata.h"
#include <QGeoCoordinate>
#include <QDebug>
#include <QVariant>

MockTelemetrySource::MockTelemetrySource(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data)
{
    m_routePoints.clear();
    // Votre tracé autour de l'IUT
    m_routePoints << QGeoCoordinate(48.269593, 4.079829)
                  << QGeoCoordinate(48.267314, 4.076437)
                  << QGeoCoordinate(48.267109, 4.076079)
                  << QGeoCoordinate(48.266902, 4.076267)
                  << QGeoCoordinate(48.265805, 4.078167)
                  << QGeoCoordinate(48.265751, 4.078509)
                  << QGeoCoordinate(48.265811, 4.078861)
                  << QGeoCoordinate(48.265971, 4.079096)
                  << QGeoCoordinate(48.266183, 4.079264)
                  << QGeoCoordinate(48.266268, 4.079301)
                  << QGeoCoordinate(48.266330, 4.079389)
                  << QGeoCoordinate(48.266389, 4.079547)
                  << QGeoCoordinate(48.266724, 4.079708)
                  << QGeoCoordinate(48.266847, 4.079909)
                  << QGeoCoordinate(48.266970, 4.080257)
                  << QGeoCoordinate(48.267120, 4.080900)
                  << QGeoCoordinate(48.267252, 4.081123)
                  << QGeoCoordinate(48.268008, 4.082088)
                  << QGeoCoordinate(48.268121, 4.082421)
                  << QGeoCoordinate(48.268126, 4.083243)
                  << QGeoCoordinate(48.268186, 4.083592)
                  << QGeoCoordinate(48.268387, 4.083990)
                  << QGeoCoordinate(48.268982, 4.084626)
                  << QGeoCoordinate(48.270582, 4.081287);

    if (!m_routePoints.isEmpty()) {
        m_currentExactPos = m_routePoints[0];
        m_currentIndex = 0;
    }

    // VITESSE DU TIMER : 50ms = 20 images par seconde (très fluide)
    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &MockTelemetrySource::tick);

    connect(m_data, &TelemetryData::simulateRouteRequested, this, [this](QVariantList routeCoords){
        if(routeCoords.isEmpty()) return;

        m_routePoints.clear();
        for(const QVariant& var : routeCoords) {
            QVariantMap pt = var.toMap();
            m_routePoints.append(QGeoCoordinate(pt["lat"].toDouble(), pt["lon"].toDouble()));
        }

        // On replace la voiture au départ du nouveau trajet
        m_currentIndex = 0;
        m_currentExactPos = m_routePoints[0];
    });
    // ==========================================
}

void MockTelemetrySource::start(){ m_timer.start(); }
void MockTelemetrySource::stop(){ m_timer.stop(); }

void MockTelemetrySource::tick(){
    if(!m_data || m_routePoints.isEmpty()) return;

    // 1. Cible actuelle
    int nextIndex = m_currentIndex + 1;
    if (nextIndex >= m_routePoints.size()) nextIndex = 0; // Boucle

    QGeoCoordinate target = m_routePoints[nextIndex];

    // 2. Calcul des distances
    double speedKmh = 50.0;
    double speedMs = speedKmh / 3.6;
    double stepDistance = speedMs * 0.05;
    double distToTarget = m_currentExactPos.distanceTo(target);

    // /!\ CORRECTION DU SAUT DE CAMÉRA /!\
    // On calcule le cap AVANT de potentiellement se téléporter sur le point
    double azimuth = m_currentExactPos.azimuthTo(target);

    if (distToTarget <= stepDistance) {
        // ON EST ARRIVÉ AU POINT
        m_currentExactPos = target;
        m_currentIndex = nextIndex;

        // On anticipe doucement l'angle du prochain segment pour fluidifier
        int nextNextIndex = nextIndex + 1;
        if (nextNextIndex < m_routePoints.size()) {
            azimuth = m_currentExactPos.azimuthTo(m_routePoints[nextNextIndex]);
        }
    } else {
        // ON AVANCE : Interpolation vers la cible
        m_currentExactPos = m_currentExactPos.atDistanceAndAzimuth(stepDistance, azimuth);
    }

    // 3. Mise à jour des données
    m_data->setLat(m_currentExactPos.latitude());
    m_data->setLon(m_currentExactPos.longitude());

    // On utilise la variable calculée plus haut qui ne tombe plus jamais à zéro !
    m_data->setHeading(azimuth);

    m_data->setSpeedKmh(speedKmh);
    m_data->setGpsOk(true);
}
