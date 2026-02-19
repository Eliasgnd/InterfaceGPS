#include "mainwindow.h"
#include "homeassistant.h"
#include "ui_mainwindow.h"
#include "telemetrydata.h"
#include "homepage.h"
#include "navigationpage.h"
#include "camerapage.h"
#include "settingspage.h"
#include "mediapage.h"

#include <QStackedWidget>

MainWindow::MainWindow(TelemetryData* telemetry, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_t(telemetry)
{
    ui->setupUi(this);

    this->setFixedSize(1280, 800);

    // 1. Création des pages
    m_home = new HomePage(this);
    m_nav = new NavigationPage(this);
    m_cam = new CameraPage(this);
    m_settings = new SettingsPage(this);
    m_media = new MediaPage(this);
    m_ha = new HomeAssistant(this);

    // 2. Connexion de la télémétrie (Sauf pour la caméra vidéo qui n'en a pas besoin)
    m_home->bindTelemetry(m_t);
    m_nav->bindTelemetry(m_t);
    // m_cam->bindTelemetry(m_t); // <--- SUPPRIMÉ car n'existe plus dans la nouvelle CameraPage
    m_settings->bindTelemetry(m_t);

    // 3. Ajout des pages au StackedWidget (Vérifie que c'est bien 'stackedPages' dans le .ui)
    ui->stackedPages->addWidget(m_home);
    ui->stackedPages->addWidget(m_nav);
    ui->stackedPages->addWidget(m_cam);
    ui->stackedPages->addWidget(m_media);
    ui->stackedPages->addWidget(m_settings);
    ui->stackedPages->addWidget(m_ha);

    // Page de démarrage
    ui->stackedPages->setCurrentWidget(m_home);

    // 4. Connexion des boutons du menu bas
    connect(ui->btnHome, &QPushButton::clicked, this, &MainWindow::goHome);
    connect(ui->btnNav, &QPushButton::clicked, this, &MainWindow::goNav);
    connect(ui->btnCam, &QPushButton::clicked, this, &MainWindow::goCam);
    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::goSettings);
    connect(ui->btnHA, &QPushButton::clicked, this, &MainWindow::goHomeAssistant);

    // Assure-toi d'avoir créé le bouton 'btnMedia' dans mainwindow.ui
    connect(ui->btnMedia, &QPushButton::clicked, this, &MainWindow::goMedia);

    // Liens rapides depuis la Home
    connect(m_home, &HomePage::requestNavigation, this, &MainWindow::goNav);
    connect(m_home, &HomePage::requestCamera, this, &MainWindow::goCam);

    // Mise à jour de la barre du haut
    updateTopBarAndAlert();
    connect(m_t, &TelemetryData::speedKmhChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::batteryPercentChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::gpsOkChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::reverseChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::alertLevelChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::alertTextChanged, this, &MainWindow::updateTopBarAndAlert);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// --- NAVIGATION ---

void MainWindow::goHome()
{
    // Important : Couper le flux vidéo si on vient de la caméra
    m_cam->stopStream();

    ui->stackedPages->setCurrentWidget(m_home);
}

void MainWindow::goNav()
{
    m_cam->stopStream();
    ui->stackedPages->setCurrentWidget(m_nav);
}

void MainWindow::goCam()
{
    // 1. Afficher la page Caméra
    ui->stackedPages->setCurrentWidget(m_cam);

    // 2. Démarrer la réception vidéo UDP
    m_cam->startStream();
}

void MainWindow::goMedia()
{
    m_cam->stopStream();
    ui->stackedPages->setCurrentWidget(m_media);
}

void MainWindow::goSettings()
{
    m_cam->stopStream();
    ui->stackedPages->setCurrentWidget(m_settings);
}

// --- BARRE DE STATUT ---

void MainWindow::updateTopBarAndAlert()
{
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

void MainWindow::goHomeAssistant() {
    m_cam->stopStream(); // Arrêt de la caméra important !

    // On change de page
    ui->stackedPages->setCurrentWidget(m_ha);

    // On force le rafraîchissement système
    QApplication::processEvents();
    m_ha->setFocus();
}
