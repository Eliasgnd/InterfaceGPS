// Rôle architectural: source de télémétrie simulée pour démonstration et tests d'interface.
// Responsabilités: générer un déplacement GPS plausible et des mesures dynamiques sans matériel réel.
// Dépendances principales: QTimer, QGeoCoordinate et TelemetryData.

#pragma once
#include <QObject>
#include <QTimer>
#include <QList>
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


    QList<QGeoCoordinate> m_route;
    int m_currentWaypointIndex = 0;
    QGeoCoordinate m_currentPos;


    double m_speedTarget = 50.0;
    double m_currentSpeed = 0.0;
    int m_batt = 100;
};
