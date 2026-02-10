#pragma once
#include <QWidget>
namespace Ui { class NavigationPage; }
class TelemetryData;

class NavigationPage : public QWidget {
    Q_OBJECT
public:
    explicit NavigationPage(QWidget* parent=nullptr);
    ~NavigationPage();
    void bindTelemetry(TelemetryData* t);

private:
    Ui::NavigationPage* ui;
    TelemetryData* m_t=nullptr;
    int m_zoom = 12;
};
