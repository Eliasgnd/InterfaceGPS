#include "mediapage.h"
#include "ui_mediapage.h"
#include <QVBoxLayout>
#include <QQmlContext>
#include "bluetoothmanager.h"

MediaPage::MediaPage(QWidget *parent) : QWidget(parent), ui(new Ui::MediaPage) {
    ui->setupUi(this);

    BluetoothManager* btManager = new BluetoothManager(this);
    m_playerView->rootContext()->setContextProperty("bluetoothManager", btManager);

    // Initialisation du QML
    m_playerView = new QQuickWidget(this);
    m_playerView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Charger le fichier QML créé à l'étape 2
    m_playerView->setSource(QUrl("qrc:/MediaPlayer.qml"));

    // Ajouter au layout (suppose que tu as un layout vertical dans ton ui)
    ui->layoutPlayer->addWidget(m_playerView);
}

MediaPage::~MediaPage() { delete ui; }
