// Rôle architectural: implémentation du pont C++/QML pour la page média.
// Responsabilités: créer la vue QML, exposer BluetoothManager au contexte et appliquer le mode d'affichage demandé.
// Dépendances principales: QQuickWidget, QQmlContext et scène MediaPlayer.qml.

#include "mediapage.h"
#include "ui_mediapage.h"
#include <QVBoxLayout>
#include <QQmlContext>
#include <QQuickItem>
#include "bluetoothmanager.h"

MediaPage::MediaPage(QWidget *parent) : QWidget(parent), ui(new Ui::MediaPage) {
    // Cette couche sert d'adaptateur entre le monde QWidget et l'écran média en QML.
    ui->setupUi(this);


    m_playerView = new QQuickWidget(this);
    m_playerView->setResizeMode(QQuickWidget::SizeRootObjectToView);


    BluetoothManager* btManager = new BluetoothManager(this);
    m_playerView->rootContext()->setContextProperty("bluetoothManager", btManager);


    m_playerView->setSource(QUrl("qrc:/MediaPlayer.qml"));


    ui->layoutPlayer->addWidget(m_playerView);
}

MediaPage::~MediaPage() { delete ui; }

void MediaPage::setCompactMode(bool compact) {
    if (m_playerView && m_playerView->rootObject()) {
        m_playerView->rootObject()->setProperty("isCompactMode", compact);
    }
}
