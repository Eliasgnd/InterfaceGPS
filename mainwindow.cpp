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
    ui->btnHome->hide(); // ON CACHE L'ANCIEN BOUTON HOME DU .UI
    // connect(ui->btnHome, &QPushButton::clicked, this, &MainWindow::goHome); // Plus besoin de le connecter

    connect(ui->btnNav, &QPushButton::clicked, this, &MainWindow::goNav);
    connect(ui->btnCam, &QPushButton::clicked, this, &MainWindow::goCam);
    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::goSettings);
    connect(ui->btnHA, &QPushButton::clicked, this, &MainWindow::goHomeAssistant);
    connect(ui->btnMedia, &QPushButton::clicked, this, &MainWindow::goMedia);

    // --- NOUVEAU BOUTON SPLIT / HOME (STYLE ANDROID AUTO) ---
    m_btnSplit = new QPushButton(this);
    m_btnSplit->setFixedSize(70, 70); // On force une taille carrée pour faire une belle icône
    m_btnSplit->setCursor(Qt::PointingHandCursor);

    // On l'insère à l'index 0 (Tout à gauche de la barre de navigation, à la place de l'ancien btnHome)
    ui->bottomNavLayout->insertWidget(0, m_btnSplit);
    connect(m_btnSplit, &QPushButton::clicked, this, &MainWindow::toggleSplitAndHome);

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

    // On prévient la page Media de se réduire (enlever le vinyle) si on est en Split
    m_media->setCompactMode(m_isSplitMode);

    // --- GESTION DU LOGO DU BOUTON (Dashboard vs Lanceur d'apps) ---
    bool isHomeVisible = (p1 == m_home || p2 == m_home);
    if (isHomeVisible) {
        // Si on est sur l'accueil, le bouton affiche l'icône du Split (Dashboard)
        m_btnSplit->setText("◫");
    } else {
        // Si on est dans une app (GPS, Musique) ou en Split, le bouton affiche le lanceur d'apps (Home)
        m_btnSplit->setText("▦");
    }

    // Style du bouton Android Auto
    m_btnSplit->setStyleSheet("QPushButton { font-size: 38px; color: white; background-color: transparent; border-radius: 12px; }"
                              "QPushButton:pressed { background-color: #2a2f3a; }");

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

// --- NOUVELLE FONCTION DE BASCULEMENT ANDROID AUTO ---
void MainWindow::toggleSplitAndHome()
{
    if (m_home->isVisible()) {
        // Si on est sur la Home, on lance le Split Screen avec nos 2 dernières apps
        goSplit();
    } else {
        // Sinon (qu'on soit en plein écran ou en split), on retourne à la Home
        goHome();
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
