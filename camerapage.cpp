/**
 * @file camerapage.cpp
 * @brief Implémentation de la page caméra du tableau de bord.
 * @details Utilise QUdpSocket pour réceptionner des images JPEG indépendantes envoyées
 * par un script externe (ex: Python/GStreamer) et les affiche en temps réel.
 */

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

    // Initialisation du pointeur vers le conteneur d'image
    videoLabel = ui->lblVideo;

    // Configuration du label pour que l'image vidéo remplisse tout l'espace disponible
    // tout en conservant son ratio (le ratio est géré en amont par le layout)
    videoLabel->setScaledContents(true);
    videoLabel->setAlignment(Qt::AlignCenter);

    qDebug() << "[CAMERA] Constructeur OK. Label vidéo connecté à l'interface.";

    // Initialisation du socket réseau UDP (sans l'ouvrir immédiatement)
    udpSocket = new QUdpSocket(this);

    // Connexion du signal de réception réseau au slot de décodage d'image
    connect(udpSocket, &QUdpSocket::readyRead, this, &CameraPage::processPendingDatagrams);
}

CameraPage::~CameraPage()
{
    delete ui;
}

void CameraPage::startStream()
{
    // On s'assure que le port n'est pas déjà ouvert avant d'essayer de s'y lier
    if (udpSocket->state() != QAbstractSocket::BoundState) {

        // Écoute sur toutes les interfaces réseau (localhost, Wi-Fi, Ethernet) sur le port 4444
        bool success = udpSocket->bind(QHostAddress::Any, 4444);

        if (success) {
            qDebug() << "CAMERA: Écoute démarrée sur le port 4444";
            videoLabel->setText("Connexion en cours...");
        } else {
            qCritical() << "CAMERA: Échec de connexion au port 4444";
            videoLabel->setText("Erreur: Port 4444 occupé");
        }
    }
}

void CameraPage::stopStream()
{
    // Libère le port réseau pour économiser les ressources système
    // et éviter le traitement en arrière-plan d'images qui ne sont pas regardées.
    if (udpSocket->isOpen()) {
        udpSocket->close();
        qDebug() << "CAMERA: Arrêt du flux";
        videoLabel->clear(); // Vide l'image courante
        videoLabel->setText("Caméra en pause");
    }
}

void CameraPage::processPendingDatagrams()
{
    // Boucle de traitement : lit tous les paquets en attente dans le buffer réseau
    while (udpSocket->hasPendingDatagrams()) {

        // Extraction de la charge utile (payload) du datagramme UDP
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();

        // Tentative de conversion des données brutes en objet QPixmap (image)
        // Le format attendu est explicitement défini à "JPG" pour accélérer le décodage
        QPixmap pixmap;
        if (pixmap.loadFromData(data, "JPG")) {
            // Si le décodage réussit, on met à jour l'affichage avec la nouvelle "frame"
            videoLabel->setPixmap(pixmap);
        } else {
            // Un paquet UDP peut être corrompu ou fragmenté (contrairement à TCP),
            // on ignore silencieusement les images cassées sans crasher l'application.
            qDebug() << "CAMERA: Image reçue invalide (paquet UDP corrompu ou incomplet)";
        }
    }
}
