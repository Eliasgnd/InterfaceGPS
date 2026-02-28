/**
 * @file mainwindow.cpp
 * @brief Implémentation de la fenêtre principale de l'application.
 * @details Gère l'initialisation du layout principal, l'instanciation des vues
 * et la logique de bascule (routing) entre ces différentes vues tout en optimisant
 * les ressources (ex: coupure du flux caméra quand il n'est pas affiché).
 */

#include "mainwindow.h"
#include "homeassistant.h"
#include "ui_mainwindow.h"
#include "telemetrydata.h"
#include "navigationpage.h"
#include "camerapage.h"
#include "settingspage.h"
#include "mediapage.h"

#ifdef ENABLE_ANDROID_AUTO
#include "androidautopage.h"
#endif

#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>

MainWindow::MainWindow(TelemetryData* telemetry, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_t(telemetry)
{
    ui->setupUi(this);

    // Format fixe : l'interface est pensée et verrouillée pour l'écran embarqué du véhicule.
    this->setFixedSize(1280, 800);

    // Configuration de la barre de navigation inférieure
    ui->bottomNavFrame->setFixedHeight(75);
    ui->bottomNavLayout->setContentsMargins(10, 2, 10, 5);
    ui->bottomNavLayout->setSpacing(15);

    // Priorité d'étirement : la zone de contenu prend tout l'espace disponible
    ui->verticalLayoutRoot->setStretch(0, 1);
    ui->verticalLayoutRoot->setStretch(1, 0);

    // Instanciation des pages classiques
    m_nav = new NavigationPage(this);
    m_cam = new CameraPage(this);
    m_settings = new SettingsPage(this);
    m_media = new MediaPage(this);
    m_ha = new HomeAssistant(this);

#ifdef ENABLE_ANDROID_AUTO
    m_aa = new AndroidAutoPage(this);
#endif

    // Restauration du dernier état de l'écran partagé (persistance utilisateur)
    QSettings settings("EliasCorp", "GPSApp");
    QString leftAppStr = settings.value("Split/Left", "Nav").toString();
    QString rightAppStr = settings.value("Split/Right", "Media").toString();

    m_lastLeftApp = stringToWidget(leftAppStr);
    m_lastRightApp = stringToWidget(rightAppStr);

    // TelemetryData sert de bus applicatif partagé.
    m_nav->bindTelemetry(m_t);

    // Configuration du conteneur principal qui va héberger toutes les pages côte à côte.
    QWidget* mainContainer = new QWidget(this);
    m_mainLayout = new QHBoxLayout(mainContainer);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    m_mainLayout->addWidget(m_nav);
    m_mainLayout->addWidget(m_cam);
    m_mainLayout->addWidget(m_media);
    m_mainLayout->addWidget(m_settings);
    m_mainLayout->addWidget(m_ha);

#ifdef ENABLE_ANDROID_AUTO
    m_mainLayout->addWidget(m_aa);
#endif

    // Remplacement de l'ancien QStackedWidget (inutilisé) par notre layout dynamique
    int stackIndex = ui->verticalLayoutRoot->indexOf(ui->stackedPages);
    ui->verticalLayoutRoot->insertWidget(stackIndex, mainContainer);
    ui->stackedPages->hide();

    // Connexion des boutons de la barre de navigation vers leurs slots respectifs
    connect(ui->btnNav, &QPushButton::clicked, this, &MainWindow::goNav);
    connect(ui->btnCam, &QPushButton::clicked, this, &MainWindow::goCam);
    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::goSettings);
    connect(ui->btnHA, &QPushButton::clicked, this, &MainWindow::goHomeAssistant);
    connect(ui->btnMedia, &QPushButton::clicked, this, &MainWindow::goMedia);

    // /!\ IMPORTANT : Assure-toi d'avoir ajouté un bouton nommé "btnAA" dans mainwindow.ui
    // avant de compiler, sinon cette ligne va générer une erreur "no member named 'btnAA'".
    // connect(ui->btnAA, &QPushButton::clicked, this, &MainWindow::goAndroidAuto);

    // Création dynamique du bouton pour activer le mode Split-Screen
    m_btnSplit = new QPushButton(this);
    m_btnSplit->setFixedSize(70, 70);
    m_btnSplit->setCursor(Qt::PointingHandCursor);

    ui->bottomNavLayout->insertWidget(0, m_btnSplit);
    connect(m_btnSplit, &QPushButton::clicked, this, &MainWindow::toggleSplitAndHome);

    // Connexion des alertes de télémétrie à l'interface globale
    connect(m_t, &TelemetryData::alertLevelChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::alertTextChanged, this, &MainWindow::updateTopBarAndAlert);

    // Démarrage de l'application directement dans l'état mémorisé (Split ou Plein écran)
    goSplit();
}

MainWindow::~MainWindow() {
    delete ui;
}

QString MainWindow::widgetToString(QWidget* w) {
#ifdef ENABLE_ANDROID_AUTO
    if (w == m_aa) return "AA";
#endif
    if (w == m_nav) return "Nav";
    if (w == m_media) return "Media";
    if (w == m_cam) return "Cam";
    if (w == m_settings) return "Settings";
    if (w == m_ha) return "HA";
    return "Nav"; // Valeur par défaut
}

QWidget* MainWindow::stringToWidget(const QString& name) {
#ifdef ENABLE_ANDROID_AUTO
    if (name == "AA") return m_aa;
#endif
    if (name == "Media") return m_media;
    if (name == "Cam") return m_cam;
    if (name == "Settings") return m_settings;
    if (name == "HA") return m_ha;
    return m_nav; // Valeur par défaut
}

void MainWindow::saveSplitState() {
    QSettings settings("EliasCorp", "GPSApp");
    settings.setValue("Split/Left", widgetToString(m_lastLeftApp));
    settings.setValue("Split/Right", widgetToString(m_lastRightApp));
}

void MainWindow::displayPages(QWidget* p1, QWidget* p2)
{
    // p1 est la page principale; si p2 n'est pas nul, on active le mode split.
    m_isSplitMode = (p2 != nullptr);

    // Le lecteur média adapte son interface (mode compact) si l'écran est partagé
    m_media->setCompactMode(m_isSplitMode);

    // Modification visuelle de l'icône du bouton Split
    m_btnSplit->setText(m_isSplitMode ? "▦" : "◫");
    m_btnSplit->setStyleSheet("QPushButton { font-size: 38px; color: white; background-color: transparent; border-radius: 12px; }"
                              "QPushButton:pressed { background-color: #2a2f3a; }");

    // Parcours de tous les widgets enfants pour afficher uniquement ceux demandés
    for (int i = 0; i < m_mainLayout->count(); ++i) {
        QWidget* w = m_mainLayout->itemAt(i)->widget();
        if (w) {
            bool shouldShow = (w == p1 || w == p2);
            w->setVisible(shouldShow);

            // Gestion des proportions en mode Split (60% gauche / 40% droite)
            if (shouldShow && m_isSplitMode) {
                if (w == p1) m_mainLayout->setStretchFactor(w, 6);
                else if (w == p2) m_mainLayout->setStretchFactor(w, 4);
            }
        }
    }
}

void MainWindow::toggleSplitAndHome() {
    // Si on n'est pas déjà en mode partagé, on l'active.
    // Si on l'est déjà, l'utilisateur devra cliquer sur une application spécifique dans la barre.
    if (!m_isSplitMode) goSplit();
}

void MainWindow::goSplit() {
    // OPTIMISATION CRITIQUE : Le flux caméra est systématiquement arrêté hors de sa page
    m_cam->stopStream();
    displayPages(m_lastLeftApp, m_lastRightApp);
}

void MainWindow::goAndroidAuto() {
    m_cam->stopStream();
#ifdef ENABLE_ANDROID_AUTO
    m_lastLeftApp = m_aa; // Définit AA comme application gauche si on passe en split-screen
    saveSplitState();
    displayPages(m_aa);
#endif
}

void MainWindow::goNav() {
    m_cam->stopStream();
    m_lastLeftApp = m_nav; // La navigation se place toujours à gauche en mode split
    saveSplitState();
    displayPages(m_nav);
}

void MainWindow::goMedia() {
    m_cam->stopStream();
    m_lastRightApp = m_media; // Le média se place toujours à droite en mode split
    saveSplitState();
    displayPages(m_media);
}

void MainWindow::goCam() {
    // La caméra prend toujours la totalité de l'écran, pas de split possible.
    displayPages(m_cam);
    m_cam->startStream();
}

void MainWindow::goSettings() {
    m_cam->stopStream();
    displayPages(m_settings);
}

void MainWindow::goHomeAssistant() {
    m_cam->stopStream();
    displayPages(m_ha);
    m_ha->setFocus();
}

void MainWindow::updateTopBarAndAlert() {
    // La top bar matérialise la criticité sécurité. Elle reste invisible en niveau 0 (tout va bien).
    if(m_t->alertLevel() == 0){
        ui->alertFrame->setVisible(false);
    } else {
        ui->alertFrame->setVisible(true);
        ui->lblAlertTitle->setText(m_t->alertLevel() == 2 ? "CRITIQUE" : "ALERTE");
        ui->lblAlertText->setText(m_t->alertText());

        // Code couleur de l'alerte : Rouge vif pour critique (2), Orange foncé pour simple alerte (1)
        ui->alertFrame->setStyleSheet(m_t->alertLevel() == 2
                                          ? "QFrame{background:#8B1E1E;border-radius:10px;} QLabel{color:white;}"
                                          : "QFrame{background:#6B4B16;border-radius:10px;} QLabel{color:white;}"
                                      );
    }
}
