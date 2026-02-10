#pragma once
#include <QWidget>
namespace Ui { class HomePage; }
class TelemetryData;

class HomePage : public QWidget {
    Q_OBJECT
public:
    explicit HomePage(QWidget* parent=nullptr);
    ~HomePage();
    void bindTelemetry(TelemetryData* t);

signals:
    void requestNavigation();
    void requestCamera();

private:
    Ui::HomePage* ui;
    TelemetryData* m_t=nullptr;
};
