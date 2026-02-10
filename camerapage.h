#pragma once
#include <QWidget>
namespace Ui { class CameraPage; }
class TelemetryData;

class CameraPage : public QWidget {
    Q_OBJECT
public:
    explicit CameraPage(QWidget* parent=nullptr);
    ~CameraPage();
    void bindTelemetry(TelemetryData* t);

private:
    void setMode(const QString& mode);

    Ui::CameraPage* ui;
    TelemetryData* m_t=nullptr;
};
