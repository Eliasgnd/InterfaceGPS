#pragma once
#include <QMainWindow>

class TelemetryData;
class HomePage;
class NavigationPage;
class CameraPage;
class SettingsPage;
class MediaPage; // <--- AJOUTER CETTE LIGNE (Forward Declaration)
class HomeAssistant;

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
    void goMedia();    // <--- AJOUTER CETTE LIGNE (Le slot existe maintenant)
    void goSettings();
    void goHomeAssistant();
    void updateTopBarAndAlert();

private:
    Ui::MainWindow* ui;
    TelemetryData* m_t=nullptr;

    HomePage* m_home=nullptr;
    NavigationPage* m_nav=nullptr;
    CameraPage* m_cam=nullptr;
    MediaPage* m_media=nullptr;       // <--- AJOUTER CETTE LIGNE
    SettingsPage* m_settings=nullptr;
    HomeAssistant* m_ha = nullptr;
};
