#pragma once
#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>

class TelemetryData;
class HomePage;
class NavigationPage;
class CameraPage;
class SettingsPage;
class MediaPage;
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
    void goMedia();
    void goSettings();
    void goHomeAssistant();
    void goSplit();
    void updateTopBarAndAlert();
    void toggleSplitAndHome();

private:
    Ui::MainWindow* ui;
    TelemetryData* m_t=nullptr;

    HomePage* m_home=nullptr;
    NavigationPage* m_nav=nullptr;
    CameraPage* m_cam=nullptr;
    MediaPage* m_media=nullptr;
    SettingsPage* m_settings=nullptr;
    HomeAssistant* m_ha = nullptr;

    // --- NOUVEAUTÉS POUR LE SPLIT SCREEN ---
    QHBoxLayout* m_mainLayout = nullptr;
    QPushButton* m_btnSplit = nullptr;
    bool m_isSplitMode = false;

    // MÉMOIRE DES DERNIÈRES APPLICATIONS OUVERTES
    QWidget* m_lastLeftApp = nullptr;  // Ex: Navigation, Waze, Maps
    QWidget* m_lastRightApp = nullptr; // Ex: Musique, Spotify, Téléphone

    // Fonction maîtresse dynamique
    void displayPages(QWidget* page1, QWidget* page2 = nullptr);
};
