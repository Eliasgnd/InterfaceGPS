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
#include <QHBoxLayout>
#include <QPushButton>

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

    // Initialisation de la mémoire Split Screen par défaut
    m_lastLeftApp = m_nav;
    m_lastRightApp = m_media;

    // 2. Connexion de la télémétrie
    m_home->bindTelemetry(m_t);
    m_nav->bindTelemetry(m_t);
    m_settings->bindTelemetry(m_t);

    // 3. Création du conteneur Split-Screen
    QWidget* mainContainer = new QWidget(this);
    m_mainLayout = new QHBoxLayout(mainContainer);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    // Ajout de toutes les pages dans ce layout horizontal
    m_mainLayout->addWidget(m_home);
    m_mainLayout->addWidget(m_nav);
    m_mainLayout->addWidget(m_cam);
    m_mainLayout->addWidget(m_media);
    m_mainLayout->addWidget(m_settings);
    m_mainLayout->addWidget(m_ha);

    // Remplacement de l'ancien QStackedWidget
    int stackIndex = ui->verticalLayoutRoot->indexOf(ui->stackedPages);
    ui->verticalLayoutRoot->insertWidget(stackIndex, mainContainer);
    ui->stackedPages->hide();

    // 4. Connexion des boutons du menu bas
    connect(ui->btnHome, &QPushButton::clicked, this, &MainWindow::goHome);
    connect(ui->btnNav, &QPushButton::clicked, this, &MainWindow::goNav);
    connect(ui->btnCam, &QPushButton::clicked, this, &MainWindow::goCam);
    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::goSettings);
    connect(ui->btnHA, &QPushButton::clicked, this, &MainWindow::goHomeAssistant);
    connect(ui->btnMedia, &QPushButton::clicked, this, &MainWindow::goMedia);

    // --- CRÉATION DU BOUTON SPLIT ---
    m_btnSplit = new QPushButton("Split", this);
    m_btnSplit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->bottomNavLayout->insertWidget(3, m_btnSplit);
    connect(m_btnSplit, &QPushButton::clicked, this, &MainWindow::goSplit);

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

    // Page de démarrage
    displayPages(m_home);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// --- LOGIQUE D'AFFICHAGE DYNAMIQUE (GÉNÉRIQUE) ---
void MainWindow::displayPages(QWidget* p1, QWidget* p2)
{
    m_isSplitMode = (p2 != nullptr);

    // NOUVEAU : On prévient la page Media de se réduire (enlever le vinyle) si on est en Split
    m_media->setCompactMode(m_isSplitMode);

    // Parcourt tous les widgets du layout automatiquement
    for (int i = 0; i < m_mainLayout->count(); ++i) {
        QWidget* w = m_mainLayout->itemAt(i)->widget();
        if (w) {
            bool shouldShow = (w == p1 || w == p2);
            w->setVisible(shouldShow);

            // Si on est en Split et que le widget doit être affiché, on partage l'écran
            if (shouldShow && m_isSplitMode) {
                // p1 (GPS) prend 60% de la place (6), p2 (Media) prend 40% (4)
                if (w == p1) {
                    m_mainLayout->setStretchFactor(w, 6);
                } else if (w == p2) {
                    m_mainLayout->setStretchFactor(w, 4);
                }
            }
        }
    }
}

// --- NAVIGATION ---

void MainWindow::goSplit()
{
    m_cam->stopStream();
    // Affiche les deux dernières apps utilisées
    displayPages(m_lastLeftApp, m_lastRightApp);
}

void MainWindow::goHome()
{
    m_cam->stopStream();
    displayPages(m_home);
}

void MainWindow::goNav()
{
    m_cam->stopStream();
    m_lastLeftApp = m_nav;  // On enregistre ce GPS comme dernière App Gauche
    displayPages(m_nav);
}

void MainWindow::goMedia()
{
    m_cam->stopStream();
    m_lastRightApp = m_media; // On enregistre ce Media comme dernière App Droite
    displayPages(m_media);
}

void MainWindow::goCam()
{
    displayPages(m_cam);
    m_cam->startStream();
}

void MainWindow::goSettings()
{
    m_cam->stopStream();
    displayPages(m_settings);
}

void MainWindow::goHomeAssistant() {
    m_cam->stopStream();
    displayPages(m_ha);
    QApplication::processEvents();
    m_ha->setFocus();
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
