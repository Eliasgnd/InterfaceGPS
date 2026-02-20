#include "mainwindow.h"
#include "homeassistant.h"
#include "ui_mainwindow.h"
#include "telemetrydata.h"
// #include "homepage.h" <--- SUPPRIMÉ
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

    // 1. On limite la hauteur de la frame qui contient les boutons
    ui->bottomNavFrame->setFixedHeight(75); // Tu peux baisser à 60 ou 70 selon tes goûts

    // 2. On réduit les marges internes pour que les boutons ne soient pas écrasés
    ui->bottomNavLayout->setContentsMargins(10, 2, 10, 5);
    ui->bottomNavLayout->setSpacing(15);

    // 3. IMPORTANT : On dit au layout vertical de TOUT donner à la zone centrale
    // Si ton layout principal dans le .ui s'appelle verticalLayoutRoot :
    ui->verticalLayoutRoot->setStretch(0, 1); // La zone des pages (index 0) prend tout le stretch
    ui->verticalLayoutRoot->setStretch(1, 0); // La barre du bas (index 1) ne s'étire pas

    // --- 1. MASQUER LA BARRE INUTILE ---
    ui->topBarFrame->hide(); // On gagne toute la hauteur !

    // 2. Création des pages (Sans HomePage)
    m_nav = new NavigationPage(this);
    m_cam = new CameraPage(this);
    m_settings = new SettingsPage(this);
    m_media = new MediaPage(this);
    m_ha = new HomeAssistant(this);

    // --- 3. CHARGEMENT DE LA MÉMOIRE (QSettings) ---
    QSettings settings("EliasCorp", "GPSApp");
    QString leftAppStr = settings.value("Split/Left", "Nav").toString();
    QString rightAppStr = settings.value("Split/Right", "Media").toString();

    m_lastLeftApp = stringToWidget(leftAppStr);
    m_lastRightApp = stringToWidget(rightAppStr);

    // 4. Connexion de la télémétrie
    m_nav->bindTelemetry(m_t);
    m_settings->bindTelemetry(m_t);

    // 5. Création du conteneur Split-Screen
    QWidget* mainContainer = new QWidget(this);
    m_mainLayout = new QHBoxLayout(mainContainer);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);

    // Ajout de toutes les pages
    m_mainLayout->addWidget(m_nav);
    m_mainLayout->addWidget(m_cam);
    m_mainLayout->addWidget(m_media);
    m_mainLayout->addWidget(m_settings);
    m_mainLayout->addWidget(m_ha);

    int stackIndex = ui->verticalLayoutRoot->indexOf(ui->stackedPages);
    ui->verticalLayoutRoot->insertWidget(stackIndex, mainContainer);
    ui->stackedPages->hide();

    // 6. Boutons
    ui->btnHome->hide();

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

    // Alertes (même si on cache le topBar, on garde l'alertFrame au cas où)
    connect(m_t, &TelemetryData::alertLevelChanged, this, &MainWindow::updateTopBarAndAlert);
    connect(m_t, &TelemetryData::alertTextChanged, this, &MainWindow::updateTopBarAndAlert);

    // --- 7. DÉMARRAGE DIRECT EN SPLIT ---
    goSplit();
}

MainWindow::~MainWindow() { delete ui; }

// --- OUTILS DE MÉMOIRE ---
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

// --- LOGIQUE D'AFFICHAGE ---
void MainWindow::displayPages(QWidget* p1, QWidget* p2)
{
    m_isSplitMode = (p2 != nullptr);
    m_media->setCompactMode(m_isSplitMode);

    // Plus de Home, le bouton affiche ◫ pour forcer le Split si on est en plein écran
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
    // Si on est en plein écran, on repasse en split
    if (!m_isSplitMode) goSplit();
}

void MainWindow::goSplit() {
    m_cam->stopStream();
    displayPages(m_lastLeftApp, m_lastRightApp);
}

void MainWindow::goNav() {
    m_cam->stopStream();
    m_lastLeftApp = m_nav;
    saveSplitState(); // <-- On sauvegarde !
    displayPages(m_nav);
}

void MainWindow::goMedia() {
    m_cam->stopStream();
    m_lastRightApp = m_media;
    saveSplitState(); // <-- On sauvegarde !
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
