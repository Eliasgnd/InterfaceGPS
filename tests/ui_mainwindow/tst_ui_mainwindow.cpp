#include <QtTest>
#include <QPushButton>
#include <QUdpSocket>

#define private public
#include "../../mainwindow.h"
#include "../../camerapage.h"
#include "../../telemetrydata.h"
#undef private

class MainWindowUiTest : public QObject
{
    Q_OBJECT

private slots:
    void startup_appliesSplitMode_andShowsNavAndMedia();
    void navButtons_switchVisiblePages();
    void splitButton_togglesIconBetweenSplitAndFullscreen();
    void splitMode_setsExpectedStretchFactors();
    void cameraOpen_thenLeaveToOtherTabs_putsCameraInPauseState();
    void openingCamera_attemptsStreamAndShowsStatusMessage();
    void rapidDoubleClicks_onNavigationKeepCoherentState();
    void realisticSequence_splitCameraSettingsSplit_isCoherent();
};

void MainWindowUiTest::startup_appliesSplitMode_andShowsNavAndMedia()
{
    TelemetryData t;
    MainWindow w(&t);

    QVERIFY(w.m_isSplitMode);
    QVERIFY(!w.m_nav->isHidden());
    QVERIFY(!w.m_media->isHidden());
    QVERIFY(w.m_cam->isHidden());
    QVERIFY(w.m_settings->isHidden());
    QVERIFY(w.m_ha->isHidden());
}

void MainWindowUiTest::navButtons_switchVisiblePages()
{
    TelemetryData t;
    MainWindow w(&t);
    w.show();

    auto* btnNav = w.findChild<QPushButton*>("btnNav");
    auto* btnCam = w.findChild<QPushButton*>("btnCam");
    auto* btnMedia = w.findChild<QPushButton*>("btnMedia");
    auto* btnSettings = w.findChild<QPushButton*>("btnSettings");
    auto* btnHA = w.findChild<QPushButton*>("btnHA");

    QVERIFY(btnNav && btnCam && btnMedia && btnSettings && btnHA);

    QTest::mouseClick(btnNav, Qt::LeftButton);
    QVERIFY(!w.m_nav->isHidden());
    QVERIFY(w.m_cam->isHidden());
    QVERIFY(w.m_media->isHidden());
    QVERIFY(w.m_settings->isHidden());
    QVERIFY(w.m_ha->isHidden());

    QTest::mouseClick(btnCam, Qt::LeftButton);
    QVERIFY(w.m_nav->isHidden());
    QVERIFY(!w.m_cam->isHidden());
    QVERIFY(w.m_media->isHidden());
    QVERIFY(w.m_settings->isHidden());
    QVERIFY(w.m_ha->isHidden());

    QTest::mouseClick(btnMedia, Qt::LeftButton);
    QVERIFY(w.m_nav->isHidden());
    QVERIFY(w.m_cam->isHidden());
    QVERIFY(!w.m_media->isHidden());
    QVERIFY(w.m_settings->isHidden());
    QVERIFY(w.m_ha->isHidden());

    QTest::mouseClick(btnSettings, Qt::LeftButton);
    QVERIFY(w.m_nav->isHidden());
    QVERIFY(w.m_cam->isHidden());
    QVERIFY(w.m_media->isHidden());
    QVERIFY(!w.m_settings->isHidden());
    QVERIFY(w.m_ha->isHidden());

    QTest::mouseClick(btnHA, Qt::LeftButton);
    QVERIFY(w.m_nav->isHidden());
    QVERIFY(w.m_cam->isHidden());
    QVERIFY(w.m_media->isHidden());
    QVERIFY(w.m_settings->isHidden());
    QVERIFY(!w.m_ha->isHidden());
}

void MainWindowUiTest::splitButton_togglesIconBetweenSplitAndFullscreen()
{
    TelemetryData t;
    MainWindow w(&t);

    QCOMPARE(w.m_btnSplit->text(), QString("▦"));

    w.goNav();
    QCOMPARE(w.m_btnSplit->text(), QString("◫"));

    w.toggleSplitAndHome();
    QCOMPARE(w.m_btnSplit->text(), QString("▦"));
}

void MainWindowUiTest::splitMode_setsExpectedStretchFactors()
{
    TelemetryData t;
    MainWindow w(&t);

    w.goSplit();

    QCOMPARE(w.m_mainLayout->stretch(w.m_mainLayout->indexOf(w.m_nav)), 6);
    QCOMPARE(w.m_mainLayout->stretch(w.m_mainLayout->indexOf(w.m_media)), 4);
}

void MainWindowUiTest::cameraOpen_thenLeaveToOtherTabs_putsCameraInPauseState()
{
    TelemetryData t;
    MainWindow w(&t);

    w.goCam();
    QVERIFY(!w.m_cam->isHidden());

    w.goNav();
    QCOMPARE(w.m_cam->videoLabel->text(), QString("Caméra en pause"));

    w.goCam();
    w.goMedia();
    QCOMPARE(w.m_cam->videoLabel->text(), QString("Caméra en pause"));

    w.goCam();
    w.goSettings();
    QCOMPARE(w.m_cam->videoLabel->text(), QString("Caméra en pause"));

    w.goCam();
    w.goHomeAssistant();
    QCOMPARE(w.m_cam->videoLabel->text(), QString("Caméra en pause"));
}

void MainWindowUiTest::openingCamera_attemptsStreamAndShowsStatusMessage()
{
    QUdpSocket probe;
    if (!probe.bind(QHostAddress::Any, 4444)) {
        QSKIP("Port 4444 occupé dans cet environnement, test de succès de bind ignoré.");
    }
    probe.close();

    TelemetryData t;
    MainWindow w(&t);
    w.goCam();

    QCOMPARE(w.m_cam->udpSocket->state(), QAbstractSocket::BoundState);
    QCOMPARE(w.m_cam->videoLabel->text(), QString("Connexion en cours..."));

    w.goNav();
}

void MainWindowUiTest::rapidDoubleClicks_onNavigationKeepCoherentState()
{
    TelemetryData t;
    MainWindow w(&t);
    w.show();

    auto* btnCam = w.findChild<QPushButton*>("btnCam");
    auto* btnSettings = w.findChild<QPushButton*>("btnSettings");
    QVERIFY(btnCam && btnSettings);

    QTest::mouseDClick(btnCam, Qt::LeftButton);
    QTest::mouseDClick(btnSettings, Qt::LeftButton);

    QVERIFY(w.m_cam->isHidden());
    QVERIFY(!w.m_settings->isHidden());
    QVERIFY(w.m_nav->isHidden());
    QVERIFY(w.m_media->isHidden());
    QVERIFY(w.m_ha->isHidden());
}

void MainWindowUiTest::realisticSequence_splitCameraSettingsSplit_isCoherent()
{
    TelemetryData t;
    MainWindow w(&t);

    w.goSplit();
    QVERIFY(w.m_isSplitMode);
    QVERIFY(!w.m_nav->isHidden());
    QVERIFY(!w.m_media->isHidden());

    w.goCam();
    QVERIFY(!w.m_cam->isHidden());
    QVERIFY(!w.m_cam->udpSocket->isOpen() || w.m_cam->videoLabel->text() == "Connexion en cours..." || w.m_cam->videoLabel->text() == "Erreur: Port 4444 occupé");

    w.goSettings();
    QVERIFY(!w.m_settings->isHidden());
    QCOMPARE(w.m_cam->videoLabel->text(), QString("Caméra en pause"));

    w.goSplit();
    QVERIFY(w.m_isSplitMode);
    QVERIFY(!w.m_nav->isHidden());
    QVERIFY(!w.m_media->isHidden());
    QVERIFY(w.m_settings->isHidden());
}

QTEST_MAIN(MainWindowUiTest)
#include "tst_ui_mainwindow.moc"
