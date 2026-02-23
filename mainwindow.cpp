// Rôle architectural: implémentation de la fenêtre principale de l'application.
// Responsabilités: initialiser les pages, connecter la navigation et synchroniser l'état global de l'interface.
// Dépendances principales: UI générée, modules page (navigation, média, caméra, réglages) et services Qt.

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
    // Format fixe: l'interface est pensée pour l'écran embarqué du véhicule.
    this->setFixedSize(1280, 800);

    ui->bottomNavFrame->setFixedHeight(75);

    ui->bottomNavLayout->setContentsMargins(10, 2, 10, 5);
    ui->bottomNavLayout->setSpacing(15);

    ui->verticalLayoutRoot->setStretch(0, 1);
    ui->verticalLayoutRoot->setStretch(1, 0);

    // Chaque page reste instanciée en permanence pour préserver son état entre les changements d'onglet.
    m_nav = new NavigationPage(this);
    m_cam = new CameraPage(this);
    m_settings = new SettingsPage(this);
    m_media = new MediaPage(this);
    m_ha = new HomeAssistant(this);

    QSettings settings("EliasCorp", "GPSApp");
    QString leftAppStr = settings.value("Split/Left", "Nav").toString();
    QString rightAppStr = settings.value("Split/Right", "Media").toString();

    m_lastLeftApp = stringToWidget(leftAppStr);
    m_lastRightApp = stringToWidget(rightAppStr);

    // TelemetryData sert de bus applicatif partagé entre les modules C++/QML.
    m_nav->bindTelemetry(m_t);

    QWidget* mainContainer = new QWidget(this);
    m_mainLayout = new QHBoxLayout(mainContainer);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    m_mainLayout->addWidget(m_nav);
    m_mainLayout->addWidget(m_cam);
    m_mainLayout->addWidget(m_media);
    m_mainLayout->addWidget(m_settings);
    m_mainLayout->addWidget(m_ha);

    int stackIndex = ui->verticalLayoutRoot->indexOf(ui->stackedPages);
    ui->verticalLayoutRoot->insertWidget(stackIndex, mainContainer);
    ui->stackedPages->hide();

    connect(ui->btnNav, &QPushButton::clicked, this, &MainWindow::goNav);
    connect(ui->btnCam, &QPushButton::clicked, this, &MainWindow::goCam);
    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::goSettings);
    connect(ui->btnHA, &QPushButton::clicked, this, &MainWindow::goHomeAssistant);
    connect(ui->btnMedia, &QPushButton::clicked, this, &MainWindow::goMedia);

    m_btnSplit = new QPushButton(this);
    m_btnSplit->setFixedSize(70, 70);
    m_btnSplit->setCursor(Qt::PointingHandCursor);

    ui->bottomNavLayout->insertWidget(0, m_btnSplit);
    connect(m_btnSplit, &QPushButton::clicked, this, &MainWindow::toggleSplitAndHome);

    connect(m_t, &TelemetryData::alertLevelChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::alertTextChanged, this, &MainWindow::updateTopBarAndAlert);

    goSplit();
}

MainWindow::~MainWindow() { delete ui; }

QString MainWindow::widgetToString(QWidget* w) {
    if (w == m_nav) return "Nav";
    if (w == m_media) return "Media";
    if (w == m_cam) return "Cam";
    if (w == m_settings) return "Settings";
    if (w == m_ha) return "HA";
    return "Nav";
}

QWidget* MainWindow::stringToWidget(const QString& name) {
    if (name == "Media") return m_media;
    if (name == "Cam") return m_cam;
    if (name == "Settings") return m_settings;
    if (name == "HA") return m_ha;
    return m_nav;
}

void MainWindow::saveSplitState() {
    QSettings settings("EliasCorp", "GPSApp");
    settings.setValue("Split/Left", widgetToString(m_lastLeftApp));
    settings.setValue("Split/Right", widgetToString(m_lastRightApp));
}

void MainWindow::displayPages(QWidget* p1, QWidget* p2)
{
    // p1 est la page principale; p2 active le mode split (écran divisé).
    m_isSplitMode = (p2 != nullptr);
    m_media->setCompactMode(m_isSplitMode);

    m_btnSplit->setText(m_isSplitMode ? "▦" : "◫");

    m_btnSplit->setStyleSheet("QPushButton { font-size: 38px; color: white; background-color: transparent; border-radius: 12px; }"
                              "QPushButton:pressed { background-color: #2a2f3a; }");

    for (int i = 0; i < m_mainLayout->count(); ++i) {
        QWidget* w = m_mainLayout->itemAt(i)->widget();
        if (w) {
            bool shouldShow = (w == p1 || w == p2);
            w->setVisible(shouldShow);

            if (shouldShow && m_isSplitMode) {
                if (w == p1) m_mainLayout->setStretchFactor(w, 6);
                else if (w == p2) m_mainLayout->setStretchFactor(w, 4);
            }
        }
    }
}

void MainWindow::toggleSplitAndHome() {
    if (!m_isSplitMode) goSplit();
}

void MainWindow::goSplit() {
    // Le flux caméra est arrêté hors de sa page pour réduire CPU et trafic réseau.
    m_cam->stopStream();
    displayPages(m_lastLeftApp, m_lastRightApp);
}

void MainWindow::goNav() {
    m_cam->stopStream();
    m_lastLeftApp = m_nav;
    saveSplitState();
    displayPages(m_nav);
}

void MainWindow::goMedia() {
    m_cam->stopStream();
    m_lastRightApp = m_media;
    saveSplitState();
    displayPages(m_media);
}

void MainWindow::goCam() {
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
    // La top bar matérialise la criticité sécurité; elle reste silencieuse en niveau 0.
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
