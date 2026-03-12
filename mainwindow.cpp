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
    ui->bottomNavFrame->setFixedHeight(40);
    ui->bottomNavLayout->setContentsMargins(10, 2, 10, 2);
    ui->bottomNavLayout->setSpacing(15);

    // Priorité d'étirement : la zone de contenu prend tout l'espace disponible
    ui->verticalLayoutRoot->setStretch(0, 1);
    ui->verticalLayoutRoot->setStretch(1, 0);

    // Instanciation des pages.
    // Chaque page est instanciée une seule fois et reste en mémoire pour préserver son état
    // (ex: ne pas avoir à recharger la carte OSM à chaque clic sur l'onglet Navigation).
    m_nav = new NavigationPage(this);
    m_cam = new CameraPage(this);
    m_settings = new SettingsPage(this);
    m_media = new MediaPage(this);
    m_ha = new HomeAssistant(this);

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

    // Remplacement de l'ancien QStackedWidget (inutilisé) par notre layout dynamique
    int stackIndex = ui->verticalLayoutRoot->indexOf(ui->stackedPages);
    ui->verticalLayoutRoot->insertWidget(stackIndex, mainContainer);
    ui->stackedPages->hide();

    // Connexion des boutons de la barre de navigation vers leurs slots respectifs
    connect(ui->btnNav, &QPushButton::pressed, this, &MainWindow::goNav);
    connect(ui->btnCam, &QPushButton::pressed, this, &MainWindow::goCam);
    connect(ui->btnSettings, &QPushButton::pressed, this, &MainWindow::goSettings);
    connect(ui->btnHA, &QPushButton::pressed, this, &MainWindow::goHomeAssistant);
    connect(ui->btnMedia, &QPushButton::pressed, this, &MainWindow::goMedia);

    // Création dynamique du bouton pour activer le mode Split-Screen
    m_btnSplit = new QPushButton(this);
    m_btnSplit->setFixedSize(45, 45);
    m_btnSplit->setCursor(Qt::PointingHandCursor);

    ui->bottomNavLayout->insertWidget(0, m_btnSplit);
    connect(m_btnSplit, &QPushButton::pressed, this, &MainWindow::toggleSplitAndHome);

    // Démarrage de l'application directement dans l'état mémorisé (Split ou Plein écran)
    goSplit();
}

MainWindow::~MainWindow() {
    delete ui;
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
    //Le flux caméra est systématiquement arrêté hors de sa page
    m_cam->stopStream();
    displayPages(m_nav,m_media);
}

void MainWindow::goNav() {
    m_cam->stopStream();
    displayPages(m_nav);
}

void MainWindow::goMedia() {
    m_cam->stopStream();
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
