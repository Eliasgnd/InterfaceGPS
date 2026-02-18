#ifndef CAMERAPAGE_H
#define CAMERAPAGE_H

#include <QWidget>
#include <QUdpSocket> // Pour recevoir les données
#include <QLabel>     // Pour afficher l'image

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
    void startStream(); // Appelé quand on arrive sur la page
    void stopStream();  // Appelé quand on quitte la page

private slots:
    void processPendingDatagrams(); // Appelé à chaque réception d'image

private:
    Ui::CameraPage *ui;
    QUdpSocket *udpSocket;
    QLabel *videoLabel;
};

#endif // CAMERAPAGE_H
