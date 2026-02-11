#pragma once
#include <QWidget>
namespace Ui { class SettingsPage; }
class TelemetryData;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent=nullptr);
    ~SettingsPage();
    void bindTelemetry(TelemetryData* t); // unused for now but consistent

    // Accessors for favorite addresses
    QString getHomeAddress() const;
    QString getWorkAddress() const;

signals:
    void themeChanged(int themeIndex); // 0 night, 1 day
    void brightnessChanged(int value);

private:
    void saveSettings();
    void loadSettings();

    Ui::SettingsPage* ui;
    TelemetryData* m_t=nullptr;
};
