#include "mediapage.h"
#include "ui_mediapage.h"
#include <QVBoxLayout>
#include <QQmlContext>
#include "bluetoothmanager.h"

MediaPage::MediaPage(QWidget *parent) : QWidget(parent), ui(new Ui::MediaPage) {
    ui->setupUi(this);

    // 1. D'ABORD : On cr�e l'objet m_playerView
    m_playerView = new QQuickWidget(this);
    m_playerView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // 2. ENSUITE : On peut acc�der � son rootContext() pour injecter le Bluetooth
    BluetoothManager* btManager = new BluetoothManager(this);
    m_playerView->rootContext()->setContextProperty("bluetoothManager", btManager);

    // 3. ENFIN : On charge l'interface QML
    m_playerView->setSource(QUrl("qrc:/MediaPlayer.qml"));

    // Ajouter au layout
    ui->layoutPlayer->addWidget(m_playerView);
}

MediaPage::~MediaPage() { delete ui; }
