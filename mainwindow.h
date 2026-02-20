#pragma once
#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings> // <--- AJOUT POUR LA SAUVEGARDE

class TelemetryData;
// class HomePage; <--- SUPPRIMÉ
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

    // HomePage* m_home=nullptr; <--- SUPPRIMÉ
    NavigationPage* m_nav=nullptr;
    CameraPage* m_cam=nullptr;
    MediaPage* m_media=nullptr;
    SettingsPage* m_settings=nullptr;
    HomeAssistant* m_ha = nullptr;

    QHBoxLayout* m_mainLayout = nullptr;
    QPushButton* m_btnSplit = nullptr;
    bool m_isSplitMode = false;

    QWidget* m_lastLeftApp = nullptr;
    QWidget* m_lastRightApp = nullptr;

    void displayPages(QWidget* page1, QWidget* page2 = nullptr);

    // --- NOUVELLES FONCTIONS DE MÉMOIRE ---
    void saveSplitState();
    QString widgetToString(QWidget* w);
    QWidget* stringToWidget(const QString& name);
};
