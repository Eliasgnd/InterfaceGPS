#pragma once
#include <QWidget>
#include <QQuickWidget>

namespace Ui { class NavigationPage; }
class TelemetryData;

class NavigationPage : public QWidget {
    Q_OBJECT
public:
    explicit NavigationPage(QWidget* parent = nullptr);
    ~NavigationPage();
    void bindTelemetry(TelemetryData* t);

private slots:
    void onRouteInfoReceived(const QString& distance, const QString& duration);

private:
    Ui::NavigationPage* ui;
    TelemetryData* m_t = nullptr;
    QQuickWidget* m_mapView = nullptr;
    double m_currentZoom = 17.0;
};
