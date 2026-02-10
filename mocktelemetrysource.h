#pragma once
#include <QObject>
#include <QTimer>

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
    TelemetryData* m_data=nullptr;
    QTimer m_timer;
    double m_t=0.0;
    int m_batt=100;
};
