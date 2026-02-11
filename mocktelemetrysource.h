#pragma once
#include <QObject>
#include <QTimer>
#include <QList>
// QGeoCoordinate est très pratique pour les calculs de distance/azimut
#include <QGeoCoordinate>

class TelemetryData;

class MockTelemetrySource : public QObject {
    Q_OBJECT
public:
    explicit MockTelemetrySource(TelemetryData* data, QObject* parent=nullptr);
    void start();
    void stop();

private slots:
    void tick();

private:
    QList<QGeoCoordinate> m_routePoints;
    int m_currentIndex = 0;
    QGeoCoordinate m_currentExactPos;

    TelemetryData* m_data = nullptr;
    QTimer m_timer;

    // Pour la simulation de route
    QList<QGeoCoordinate> m_route; // Liste des points GPS du trajet
    int m_currentWaypointIndex = 0; // Point qu'on essaie d'atteindre
    QGeoCoordinate m_currentPos;    // Position actuelle exacte

    // Simulation batterie/vitesse
    double m_speedTarget = 50.0;
    double m_currentSpeed = 0.0;
    int m_batt = 100;
};
