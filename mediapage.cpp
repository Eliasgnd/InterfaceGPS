/**
 * @file mediapage.cpp
 * @brief Implémentation du pont C++/QML pour la page média.
 * @details Responsabilités : Créer la vue QML, instancier le BluetoothManager,
 * l'exposer au contexte d'exécution QML, et relayer les commandes d'affichage (mode compact).
 * Dépendances principales : QQuickWidget, QQmlContext et la scène MediaPlayer.qml.
 */

#include "mediapage.h"
#include "ui_mediapage.h"
#include <QVBoxLayout>
#include <QQmlContext>
#include <QQuickItem>
#include "bluetoothmanager.h"

MediaPage::MediaPage(QWidget *parent) : QWidget(parent), ui(new Ui::MediaPage) {
    // Cette couche C++ sert d'adaptateur entre le monde QWidget (l'application principale)
    // et l'écran média rendu en QML.
    ui->setupUi(this);

    // Initialisation du conteneur QML
    m_playerView = new QQuickWidget(this);
    m_playerView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // --- INJECTION DE DÉPENDANCES (Dependency Injection) ---
    // On crée l'instance du gestionnaire matériel Bluetooth côté C++
    BluetoothManager* btManager = new BluetoothManager(this);

    // On expose ce gestionnaire au moteur QML sous le nom "bluetoothManager".
    // Cela permet au code Javascript/QML du fichier MediaPlayer.qml d'appeler directement
    // les méthodes C++ (ex: bluetoothManager.togglePlay()) et de lire ses propriétés.
    m_playerView->rootContext()->setContextProperty("bluetoothManager", btManager);

    // Chargement du fichier source QML depuis les ressources de l'application
    m_playerView->setSource(QUrl("qrc:/MediaPlayer.qml"));

    // Insertion du lecteur dans la hiérarchie visuelle de la page
    ui->layoutPlayer->addWidget(m_playerView);
}

MediaPage::~MediaPage() {
    delete ui;
}

void MediaPage::setCompactMode(bool compact) {
    // Transmission dynamique de la commande d'affichage vers le monde QML.
    // On modifie une propriété ("isCompactMode") directement à la racine de la scène QML.
    if (m_playerView && m_playerView->rootObject()) {
        m_playerView->rootObject()->setProperty("isCompactMode", compact);
    }
}
