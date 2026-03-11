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

void CameraPageUiTest::processPendingDatagrams_invalidJpeg_keepsTextMessage()
{
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
