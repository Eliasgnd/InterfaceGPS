#include <QtTest>
#include <QPushButton>
#include <QUdpSocket>

#define private public
#include "../../mainwindow.h"
#include "../../camerapage.h"
#include "../../telemetrydata.h"
#include "../../navigationpage.h"
#include "../../mediapage.h"
#include "../../settingspage.h"
#include "../../homeassistant.h"
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
    // Objectif: valider l'état initial global de la fenêtre principale.
    // Pourquoi: cet état conditionne la première impression utilisateur et le parcours par défaut.
    // Procédure détaillée:
    //   1) Construire MainWindow avec TelemetryData.
    //   2) Vérifier m_isSplitMode=true.
    //   3) Vérifier Navigation/Media visibles et les autres pages masquées.
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
    // Objectif: valider le routage UI piloté par la barre de boutons.
    // Pourquoi: chaque bouton doit activer exactement une page pour éviter les recouvrements d'écran.
    // Procédure détaillée:
    //   1) Récupérer les boutons via objectName.
    //   2) Simuler un clic utilisateur sur chaque bouton dans l'ordre.
    //   3) Après chaque clic, vérifier la matrice de visibilité (1 visible, autres cachées).
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
    // Objectif: vérifier le feedback visuel du bouton split/fullscreen.
    // Pourquoi: l'icône doit refléter l'action possible pour ne pas tromper l'utilisateur.
    // Procédure détaillée:
    //   1) Contrôler l'icône initiale en mode split.
    //   2) Basculer en mode navigation (plein écran).
    //   3) Vérifier le changement d'icône, puis retour split et validation inverse.
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
    // Objectif: garantir la répartition visuelle prévue entre Navigation et Media.
    // Pourquoi: une régression de stretch casse l'ergonomie de lecture carte/média.
    // Procédure détaillée:
    //   1) Forcer goSplit().
    //   2) Lire les stretch factors dans le layout principal.
    //   3) Vérifier le ratio attendu (6 pour nav, 4 pour media).
    TelemetryData t;
    MainWindow w(&t);

    w.goSplit();

    QCOMPARE(w.m_mainLayout->stretch(w.m_mainLayout->indexOf(w.m_nav)), 6);
    QCOMPARE(w.m_mainLayout->stretch(w.m_mainLayout->indexOf(w.m_media)), 4);
}

void MainWindowUiTest::cameraOpen_thenLeaveToOtherTabs_putsCameraInPauseState()
{
    // Objectif: vérifier la politique d'arrêt caméra hors onglet Camera.
    // Pourquoi: économiser ressources et éviter un flux actif non visible.
    // Procédure détaillée:
    //   1) Entrer dans Camera puis naviguer successivement vers Nav/Media/Settings/HA.
    //   2) Après chaque sortie, vérifier que le label caméra indique "Caméra en pause".
    //   3) Répéter l'ouverture intermédiaire pour tester la stabilité du comportement.
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
    // Objectif: valider le scénario d'ouverture caméra depuis MainWindow.
    // Pourquoi: ce test couvre l'intégration entre orchestration UI et composant CameraPage.
    // Procédure détaillée:
    //   1) Tenter de binder 4444 en amont: si impossible, ignorer le test proprement (QSKIP).
    //   2) Ouvrir l'onglet caméra via goCam().
    //   3) Vérifier socket bound + message "Connexion en cours...".
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
    // Objectif: tester la robustesse aux interactions rapides (double-clic).
    // Pourquoi: les utilisateurs peuvent cliquer vite; l'état final doit rester déterministe.
    // Procédure détaillée:
    //   1) Simuler un double-clic sur Camera puis Settings.
    //   2) Vérifier que Settings est l'écran final visible.
    //   3) Vérifier que toutes les autres pages sont masquées.
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
    // Objectif: valider une séquence complète proche d'un usage réel.
    // Pourquoi: les bugs d'intégration apparaissent souvent dans les transitions en chaîne.
    // Procédure détaillée:
    //   1) Passer en split et vérifier nav+media.
    //   2) Ouvrir caméra et accepter les états possibles selon disponibilité du port.
    //   3) Aller dans Settings, vérifier pause caméra.
    //   4) Revenir en split et vérifier cohérence finale des visibilités.
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
