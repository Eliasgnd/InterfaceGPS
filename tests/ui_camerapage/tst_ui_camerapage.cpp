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
    CameraPage page;

    page.startStream();

    QCOMPARE(page.udpSocket->state(), QAbstractSocket::BoundState);
    QCOMPARE(page.videoLabel->text(), QString("Connexion en cours..."));

    page.stopStream();
}

void CameraPageUiTest::startStream_whenPortOccupied_setsErrorMessage()
{
    QUdpSocket blocker;
    QVERIFY(blocker.bind(QHostAddress::Any, 4444));

    CameraPage page;
    page.startStream();

    QVERIFY(page.udpSocket->state() != QAbstractSocket::BoundState);
    QCOMPARE(page.videoLabel->text(), QString("Erreur: Port 4444 occupé"));
}

void CameraPageUiTest::stopStream_afterStart_closesSocketAndSetsPausedMessage()
{
    CameraPage page;
    page.startStream();
    QCOMPARE(page.udpSocket->state(), QAbstractSocket::BoundState);

    page.stopStream();

    QVERIFY(!page.udpSocket->isOpen());
    QCOMPARE(page.videoLabel->text(), QString("Caméra en pause"));
}

QTEST_MAIN(CameraPageUiTest)
#include "tst_ui_camerapage.moc"
