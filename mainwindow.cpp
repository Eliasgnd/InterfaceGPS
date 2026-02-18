#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "telemetrydata.h"
#include "homepage.h"
#include "navigationpage.h"
#include "camerapage.h"
#include "settingspage.h"
#include "mediapage.h" // <--- 1. AJOUT DE L'INCLUDE

#include <QStackedWidget>

MainWindow::MainWindow(TelemetryData* telemetry, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_t(telemetry)
{
    ui->setupUi(this);

    // Create pages
    m_home = new HomePage(this);
    m_nav = new NavigationPage(this);
    m_cam = new CameraPage(this);
    m_settings = new SettingsPage(this);
    m_media = new MediaPage(this); // <--- 2. CRÉATION DE LA PAGE MEDIA

    // Bind telemetry
    m_home->bindTelemetry(m_t);
    m_nav->bindTelemetry(m_t);
    m_cam->bindTelemetry(m_t);
    m_settings->bindTelemetry(m_t);
    // m_media->bindTelemetry(m_t); // Décommente si tu ajoutes bindTelemetry dans MediaPage plus tard

    // Put into stacked widget
    ui->stackedPages->addWidget(m_home);
    ui->stackedPages->addWidget(m_nav);
    ui->stackedPages->addWidget(m_cam);
    ui->stackedPages->addWidget(m_media); // <--- 3. AJOUT AU STACK
    ui->stackedPages->addWidget(m_settings);

    ui->stackedPages->setCurrentWidget(m_home);

    // Bottom nav buttons
    connect(ui->btnHome, &QPushButton::clicked, this, &MainWindow::goHome);
    connect(ui->btnNav, &QPushButton::clicked, this, &MainWindow::goNav);
    connect(ui->btnCam, &QPushButton::clicked, this, &MainWindow::goCam);
    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::goSettings);

    // --- 4. CONNEXION DU BOUTON MEDIA ---
    // Note : Assure-toi d'avoir ajouté un bouton nommé "btnMedia" dans ton mainwindow.ui
    // Sinon, cette ligne fera planter la compilation. Décommente-la une fois le bouton créé.
    connect(ui->btnMedia, &QPushButton::clicked, this, &MainWindow::goMedia);

    // quick links from home
    connect(m_home, &HomePage::requestNavigation, this, &MainWindow::goNav);
    connect(m_home, &HomePage::requestCamera, this, &MainWindow::goCam);

    // Update topbar + alert
    updateTopBarAndAlert();
    connect(m_t, &TelemetryData::speedKmhChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::batteryPercentChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::gpsOkChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::reverseChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::alertLevelChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::alertTextChanged, this, &MainWindow::updateTopBarAndAlert);
}

MainWindow::~MainWindow(){ delete ui; }

void MainWindow::goHome(){ ui->stackedPages->setCurrentWidget(m_home); }
void MainWindow::goNav(){ ui->stackedPages->setCurrentWidget(m_nav); }
void MainWindow::goCam(){ ui->stackedPages->setCurrentWidget(m_cam); }
void MainWindow::goSettings(){ ui->stackedPages->setCurrentWidget(m_settings); }
void MainWindow::goMedia(){ ui->stackedPages->setCurrentWidget(m_media); } // <--- 5. FONCTION POUR AFFICHER

void MainWindow::updateTopBarAndAlert(){
    ui->lblSpeed->setText(QString("%1 km/h").arg((int)qRound(m_t->speedKmh())));
    ui->lblBattery->setText(QString("Bat %1%").arg(m_t->batteryPercent()));
    ui->lblGps->setText(m_t->gpsOk() ? "GPS OK" : "GPS LOST");
    ui->lblGear->setText(m_t->reverse() ? "R" : "D");

    if(m_t->alertLevel() == 0){
        ui->alertFrame->setVisible(false);
    } else {
        ui->alertFrame->setVisible(true);
        ui->lblAlertTitle->setText(m_t->alertLevel()==2 ? "CRITIQUE" : "ALERTE");
        ui->lblAlertText->setText(m_t->alertText());
        ui->alertFrame->setStyleSheet(m_t->alertLevel()==2
                                          ? "QFrame{background:#8B1E1E;border-radius:10px;} QLabel{color:white;}"
                                          : "QFrame{background:#6B4B16;border-radius:10px;} QLabel{color:white;}"
                                      );
    }
}
