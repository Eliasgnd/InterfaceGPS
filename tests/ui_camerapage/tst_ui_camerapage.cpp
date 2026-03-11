#include <QtTest>
#include <QUdpSocket>
#include <QLabel>
#include <QBuffer>
#include <QImage>

#define private public
#include "../../camerapage.h"
#undef private

class CameraPageUiTest : public QObject
{
    Q_OBJECT

private slots:
    void startStream_whenPortAvailable_setsConnectingMessage();
    void startStream_whenPortOccupied_setsErrorMessage();
    void stopStream_afterStart_closesSocketAndSetsPausedMessage();
    void processPendingDatagrams_invalidJpeg_keepsTextMessage();
    void processPendingDatagrams_validJpeg_setsPixmap();
};

void CameraPageUiTest::startStream_whenPortAvailable_setsConnectingMessage()
{
    // Objectif: vérifier le démarrage nominal du flux caméra UDP.
    // Pourquoi: c'est l'état attendu quand le port réseau est libre.
    // Procédure détaillée:
    //   1) Créer CameraPage puis appeler startStream().
    //   2) Vérifier que le socket est bien "Bound" (attaché au port 4444).
    //   3) Vérifier le message utilisateur "Connexion en cours...".
    CameraPage page;

    page.startStream();

    QCOMPARE(page.udpSocket->state(), QAbstractSocket::BoundState);
    QCOMPARE(page.videoLabel->text(), QString("Connexion en cours..."));

    page.stopStream();
}

void CameraPageUiTest::startStream_whenPortOccupied_setsErrorMessage()
{
    // Objectif: couvrir le cas d'erreur "port déjà utilisé".
    // Pourquoi: l'utilisateur doit recevoir un message explicite en cas de conflit réseau.
    // Procédure détaillée:
    //   1) Ouvrir d'abord un socket de blocage sur 4444.
    //   2) Lancer startStream() sur CameraPage.
    //   3) Vérifier que la page ne passe pas en BoundState et affiche le message d'erreur.
    QUdpSocket blocker;
    QVERIFY(blocker.bind(QHostAddress::Any, 4444));

    CameraPage page;
    page.startStream();

    QVERIFY(page.udpSocket->state() != QAbstractSocket::BoundState);
    QCOMPARE(page.videoLabel->text(), QString("Erreur: Port 4444 occupé"));
}

void CameraPageUiTest::stopStream_afterStart_closesSocketAndSetsPausedMessage()
{
    // Objectif: valider l'arrêt propre du flux caméra.
    // Pourquoi: quitter l'écran caméra doit libérer les ressources système.
    // Procédure détaillée:
    //   1) Démarrer le stream.
    //   2) Appeler stopStream().
    //   3) Vérifier fermeture socket + texte "Caméra en pause".
    CameraPage page;
    page.startStream();
    QCOMPARE(page.udpSocket->state(), QAbstractSocket::BoundState);

    page.stopStream();

    QVERIFY(!page.udpSocket->isOpen());
    QCOMPARE(page.videoLabel->text(), QString("Caméra en pause"));
}

void CameraPageUiTest::processPendingDatagrams_invalidJpeg_keepsTextMessage()
{
    // Objectif: tester la robustesse face à un datagramme non image.
    // Pourquoi: en réseau, des paquets corrompus/inattendus peuvent arriver.
    // Procédure détaillée:
    //   1) Démarrer le stream et envoyer "not-a-jpeg" sur le port caméra.
    //   2) Attendre brièvement le traitement asynchrone.
    //   3) Vérifier qu'aucun pixmap valide n'est affiché et que le texte reste inchangé.
    CameraPage page;
    page.startStream();
    QVERIFY(page.udpSocket->state() == QAbstractSocket::BoundState);

    QUdpSocket sender;
    sender.writeDatagram("not-a-jpeg", QHostAddress::LocalHost, 4444);
    QTest::qWait(50);

    QVERIFY(page.videoLabel->pixmap() == nullptr || page.videoLabel->pixmap()->isNull());
    QCOMPARE(page.videoLabel->text(), QString("Connexion en cours..."));

    page.stopStream();
}

void CameraPageUiTest::processPendingDatagrams_validJpeg_setsPixmap()
{
    // Objectif: vérifier le décodage et l'affichage d'une image JPEG valide.
    // Pourquoi: c'est la fonctionnalité principale de la page caméra.
    // Procédure détaillée:
    //   1) Générer une petite image en mémoire puis l'encoder en JPG.
    //   2) Envoyer les octets via UDP vers 127.0.0.1:4444.
    //   3) Vérifier qu'un pixmap non nul est présent dans le label vidéo.
    CameraPage page;
    page.startStream();
    QVERIFY(page.udpSocket->state() == QAbstractSocket::BoundState);

    QImage img(8, 8, QImage::Format_RGB32);
    img.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(img.save(&buffer, "JPG"));

    QUdpSocket sender;
    sender.writeDatagram(bytes, QHostAddress::LocalHost, 4444);
    QTest::qWait(50);

    QVERIFY(page.videoLabel->pixmap() != nullptr);
    QVERIFY(!page.videoLabel->pixmap()->isNull());

    page.stopStream();
}

QTEST_MAIN(CameraPageUiTest)
#include "tst_ui_camerapage.moc"
