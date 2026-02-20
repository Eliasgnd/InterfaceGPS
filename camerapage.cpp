// Rôle architectural: implémentation de la page caméra du tableau de bord.
// Responsabilités: ouvrir/fermer l'écoute réseau et convertir les datagrammes JPEG en image affichable.
// Dépendances principales: Qt Network, Qt Widgets et pipeline UDP local.

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


    videoLabel = ui->lblVideo;

    videoLabel->setScaledContents(true);
    videoLabel->setAlignment(Qt::AlignCenter);

    qDebug() << "[CAMERA] Constructeur OK. Label vidéo connecté à l'interface.";

    udpSocket = new QUdpSocket(this);
    connect(udpSocket, &QUdpSocket::readyRead, this, &CameraPage::processPendingDatagrams);
}
CameraPage::~CameraPage()
{
    delete ui;
}

void CameraPage::startStream()
{

    if (udpSocket->state() != QAbstractSocket::BoundState) {

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

    if (udpSocket->isOpen()) {
        udpSocket->close();
        qDebug() << "CAMERA: Arrêt du flux";
        videoLabel->setText("Caméra en pause");
        videoLabel->clear();
    }
}

void CameraPage::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {

        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();


        QPixmap pixmap;
        if (pixmap.loadFromData(data, "JPG")) {

            videoLabel->setPixmap(pixmap);
        } else {

             qDebug() << "CAMERA: Image reçue invalide";
        }
    }
}
