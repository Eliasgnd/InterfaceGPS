#include <QtTest>
#include <QUdpSocket>
#include <QLabel>

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
};

void CameraPageUiTest::startStream_whenPortAvailable_setsConnectingMessage()
{
    // Objectif: valider le scénario nominal d'ouverture du flux caméra.
    // Pourquoi: au premier affichage caméra, l'application doit binder le port vidéo local.
    // Procédure détaillée:
    //   1) Créer CameraPage.
    //   2) Appeler startStream().
    //   3) Vérifier que le socket est en BoundState et que le message utilisateur est "Connexion en cours...".
    CameraPage page;

    page.startStream();

    QCOMPARE(page.udpSocket->state(), QAbstractSocket::BoundState);
    QCOMPARE(page.videoLabel->text(), QString("Connexion en cours..."));

    page.stopStream();
}

void CameraPageUiTest::startStream_whenPortOccupied_setsErrorMessage()
{
    // Objectif: tester la gestion de conflit de port (4444 déjà occupé).
    // Pourquoi: ce cas est fréquent si un autre process consomme le flux UDP.
    // Procédure détaillée:
    //   1) Ouvrir un socket blocker qui bind explicitement le port 4444.
    //   2) Lancer startStream() sur CameraPage.
    //   3) Vérifier l'échec de bind côté page + message d'erreur explicite à l'utilisateur.
    QUdpSocket blocker;
    QVERIFY(blocker.bind(QHostAddress::Any, 4444));

    CameraPage page;
    page.startStream();

    QVERIFY(page.udpSocket->state() != QAbstractSocket::BoundState);
    QCOMPARE(page.videoLabel->text(), QString("Erreur: Port 4444 occupé"));
}

void CameraPageUiTest::stopStream_afterStart_closesSocketAndSetsPausedMessage()
{
    // Objectif: valider le nettoyage lors de l'arrêt du stream.
    // Pourquoi: lors d'un changement d'onglet, la caméra doit libérer les ressources réseau.
    // Procédure détaillée:
    //   1) Démarrer le stream et vérifier l'état bound.
    //   2) Appeler stopStream().
    //   3) Vérifier fermeture du socket et affichage "Caméra en pause".
    CameraPage page;
    page.startStream();
    QCOMPARE(page.udpSocket->state(), QAbstractSocket::BoundState);

    page.stopStream();

    QVERIFY(!page.udpSocket->isOpen());
    QCOMPARE(page.videoLabel->text(), QString("Caméra en pause"));
}

QTEST_MAIN(CameraPageUiTest)
#include "tst_ui_camerapage.moc"
