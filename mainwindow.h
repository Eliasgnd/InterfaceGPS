#pragma once
#include <QMainWindow>

class TelemetryData;
class HomePage;
class NavigationPage;
class CameraPage;
class SettingsPage;

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(TelemetryData* telemetry, QWidget* parent=nullptr);
    ~MainWindow();

private slots:
    void goHome();
    void goNav();
    void goCam();
    void goSettings();
    void updateTopBarAndAlert();

private:
    Ui::MainWindow* ui;
    TelemetryData* m_t=nullptr;

    HomePage* m_home=nullptr;
    NavigationPage* m_nav=nullptr;
    CameraPage* m_cam=nullptr;
    SettingsPage* m_settings=nullptr;
};
