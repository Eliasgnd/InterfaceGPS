#include "camerapage.h"
#include "ui_camerapage.h"
#include <QVBoxLayout>
#include <QPixmap>
#include <QDebug>
#include <QNetworkDatagram>

CameraPage::CameraPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraPage)
{
    ui->setupUi(this);

    // MODIFICATION ICI : On utilise 'this' au lieu de 'ui->frameCamera'
    // Cela permet d'afficher la vidéo en plein écran dans la page
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);

    videoLabel = new QLabel(this);
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setStyleSheet("background-color: black; color: white; font-size: 18px;");
    videoLabel->setText("En attente du signal vidéo...\n(Port 4444)");
    videoLabel->setScaledContents(true);

    layout->addWidget(videoLabel);

    // 2. PRÉPARATION DU RÉSEAU
    udpSocket = new QUdpSocket(this);
    connect(udpSocket, &QUdpSocket::readyRead, this, &CameraPage::processPendingDatagrams);
}

CameraPage::~CameraPage()
{
    delete ui;
}

void CameraPage::startStream()
{
    // On vérifie si on n'écoute pas déjà
    if (udpSocket->state() != QAbstractSocket::BoundState) {
        // QHostAddress::Any = On accepte les images venant de n'importe qui (PC, Pi, Tel)
        bool success = udpSocket->bind(QHostAddress::Any, 4444);

        if (success) {
            qDebug() << "CAMERA: Écoute démarée sur le port 4444";
            videoLabel->setText("Connexion en cours...");
        } else {
            qDebug() << "CAMERA: Échec de connexion au port 4444";
            videoLabel->setText("Erreur: Port 4444 occupé");
        }
    }
}

void CameraPage::stopStream()
{
    // On ferme la connexion pour économiser la batterie/CPU quand on n'est pas sur la page
    if (udpSocket->isOpen()) {
        udpSocket->close();
        qDebug() << "CAMERA: Arrêt du flux";
        videoLabel->setText("Caméra en pause");
        videoLabel->clear(); // Efface la dernière image
    }
}

void CameraPage::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        // 1. Récupération du paquet brut
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();

        // 2. Conversion des données (JPG) en Image Qt
        QPixmap pixmap;
        if (pixmap.loadFromData(data, "JPG")) {
            // 3. Affichage
            videoLabel->setPixmap(pixmap);
        } else {
            // Si l'image est corrompue (paquet incomplet), on ignore
             qDebug() << "CAMERA: Image reçue invalide";
        }
    }
}
