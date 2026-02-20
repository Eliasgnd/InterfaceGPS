// Rôle architectural: page UI dédiée au flux caméra embarqué.
// Responsabilités: gérer le cycle de vie de l'écoute UDP et l'affichage du dernier frame reçu.
// Dépendances principales: QWidget, QUdpSocket, QLabel et l'UI générée par Qt Designer.

#ifndef CAMERAPAGE_H
#define CAMERAPAGE_H

#include <QWidget>
#include <QUdpSocket>
#include <QLabel>

namespace Ui {
class CameraPage;
}

class CameraPage : public QWidget
{
    Q_OBJECT

public:
    explicit CameraPage(QWidget *parent = nullptr);
    ~CameraPage();

public slots:
    void startStream();
    void stopStream();

private slots:
    void processPendingDatagrams();

private:
    Ui::CameraPage *ui;
    QUdpSocket *udpSocket;
    QLabel *videoLabel;
};

#endif
