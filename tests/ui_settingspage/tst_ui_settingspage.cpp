#include <QtTest>
#include <QPushButton>
#include <QListWidget>
#include <QTemporaryDir>
#include <QFile>

#include "ui_settingspage.h"

#define private public
#include "../../settingspage.h"
#undef private

class SettingsPageUiTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void constructor_initialState_forgetButtonDisabled();
    void setDiscoverable_true_updatesButtonAndTimer();
    void setDiscoverable_false_updatesButtonAndTimer();
    void stopDiscovery_alwaysResetsVisibleState();
    void refreshPairedList_withNoSelection_keepsForgetDisabled();
    void listSelection_withValidMac_enablesForgetButton();
    void refreshPairedList_restoresSelection();
    void refreshPairedList_multipleConnected_keepsOnlyNewcomer();

private:
    QTemporaryDir m_tempDir;
    QByteArray m_originalPath;

    void installFakeBluetoothctl();
    QString readLog() const;
};

void SettingsPageUiTest::installFakeBluetoothctl()
{
    QVERIFY(m_tempDir.isValid());

    const QString scriptPath = m_tempDir.path() + "/bluetoothctl";
    QFile script(scriptPath);
    QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));

    const QByteArray content = R"(#!/usr/bin/env bash
set -e
LOG_FILE="${BT_LOG:-/tmp/bt.log}"
MODE="${BT_MODE:-none}"
cmd="$1"
shift || true

case "$cmd" in
  devices)
    if [ "$MODE" = "single" ]; then
      echo "Device AA:BB:CC:DD:EE:01 Pixel 7"
    elif [ "$MODE" = "multi" ]; then
      echo "Device AA:BB:CC:DD:EE:01 Pixel 7"
      echo "Device AA:BB:CC:DD:EE:02 iPhone 15"
    fi
    ;;
  info)
    mac="$1"
    if [ "$MODE" = "single" ] && [ "$mac" = "AA:BB:CC:DD:EE:01" ]; then
      echo "Connected: yes"
    elif [ "$MODE" = "multi" ]; then
      if [ "$mac" = "AA:BB:CC:DD:EE:01" ] || [ "$mac" = "AA:BB:CC:DD:EE:02" ]; then
        echo "Connected: yes"
      else
        echo "Connected: no"
      fi
    else
      echo "Connected: no"
    fi
    ;;
  *)
    echo "$cmd $*" >> "$LOG_FILE"
    ;;
esac
)";

    script.write(content);
    script.close();
    QVERIFY(QFile::setPermissions(scriptPath,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                  QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                  QFileDevice::ReadOther | QFileDevice::ExeOther));

    m_originalPath = qgetenv("PATH");
    const QByteArray fakePath = (m_tempDir.path() + ":" + QString::fromLocal8Bit(m_originalPath)).toLocal8Bit();
    qputenv("PATH", fakePath);

    const QByteArray logPath = (m_tempDir.path() + "/bt.log").toLocal8Bit();
    qputenv("BT_LOG", logPath);
    QFile::remove(QString::fromLocal8Bit(logPath));
}

QString SettingsPageUiTest::readLog() const
{
    QFile logFile(m_tempDir.path() + "/bt.log");
    if (!logFile.exists()) {
        return QString();
    }
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(logFile.readAll());
}

void SettingsPageUiTest::initTestCase()
{
    installFakeBluetoothctl();
}

void SettingsPageUiTest::cleanupTestCase()
{
    qputenv("PATH", m_originalPath);
}

void SettingsPageUiTest::constructor_initialState_forgetButtonDisabled()
{
    // Objectif: vérifier l'état initial sécurisé du bouton "Oublier".
    // Pourquoi: sans appareil sélectionné, ce bouton doit rester inactif.
    // Procédure détaillée:
    //   1) Simuler un environnement bluetooth sans device (BT_MODE=none).
    //   2) Construire SettingsPage.
    //   3) Vérifier que btnForget est désactivé.
    qputenv("BT_MODE", "none");
    SettingsPage page;

    QVERIFY(!page.ui->btnForget->isEnabled());
}

void SettingsPageUiTest::setDiscoverable_true_updatesButtonAndTimer()
{
    // Objectif: contrôler l'activation du mode appairage visible.
    // Pourquoi: l'utilisateur doit voir un état UI cohérent + un timeout de sécurité.
    // Procédure détaillée:
    //   1) Appeler setDiscoverable(true).
    //   2) Vérifier bouton coché, libellé "Visible (120s max)..." et timer actif.
    //   3) Vérifier dans le log la commande "discoverable on" envoyée au système.
    qputenv("BT_MODE", "none");
    SettingsPage page;

    page.setDiscoverable(true);

    QVERIFY(page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Visible (120s max)..."));
    QVERIFY(page.m_discoveryTimer->isActive());

    const QString log = readLog();
    QVERIFY(log.contains("discoverable on"));
}

void SettingsPageUiTest::setDiscoverable_false_updatesButtonAndTimer()
{
    // Objectif: contrôler le retour au mode non visible.
    // Pourquoi: sortir de l'appairage doit stopper le timer et refléter l'état en UI.
    // Procédure détaillée:
    //   1) Activer puis désactiver discoverable.
    //   2) Vérifier bouton décoché, texte par défaut et timer arrêté.
    //   3) Vérifier la commande "discoverable off" dans le log fake bluetoothctl.
    qputenv("BT_MODE", "none");
    SettingsPage page;
    page.setDiscoverable(true);

    page.setDiscoverable(false);

    QVERIFY(!page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Rendre Visible (Appairage)"));
    QVERIFY(!page.m_discoveryTimer->isActive());

    const QString log = readLog();
    QVERIFY(log.contains("discoverable off"));
}

void SettingsPageUiTest::stopDiscovery_alwaysResetsVisibleState()
{
    // Objectif: valider l'action d'arrêt forcé de la découverte.
    // Pourquoi: appelée par timer ou navigation, elle doit toujours remettre un état propre.
    // Procédure détaillée:
    //   1) Partir d'un état discoverable actif.
    //   2) Appeler stopDiscovery().
    //   3) Vérifier bouton non coché + texte standard d'appairage.
    qputenv("BT_MODE", "none");
    SettingsPage page;
    page.setDiscoverable(true);
    QVERIFY(page.ui->btnVisible->isChecked());

    page.stopDiscovery();

    QVERIFY(!page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Rendre Visible (Appairage)"));
}

void SettingsPageUiTest::refreshPairedList_withNoSelection_keepsForgetDisabled()
{
    // Objectif: vérifier le rafraîchissement de liste sans sélection courante.
    // Pourquoi: éviter toute action destructive tant qu'aucun appareil n'est explicitement choisi.
    // Procédure détaillée:
    //   1) Rafraîchir la liste en mode sans appareils connectés.
    //   2) Vérifier que le bouton "Oublier" reste désactivé.
    //   3) Vérifier qu'au moins une entrée informative est affichée dans la liste.
    qputenv("BT_MODE", "none");
    SettingsPage page;

    page.refreshPairedList();

    QVERIFY(!page.ui->btnForget->isEnabled());
    QVERIFY(page.ui->listDevices->count() >= 1);
}

void SettingsPageUiTest::listSelection_withValidMac_enablesForgetButton()
{
    // Objectif: confirmer qu'une sélection valide active l'action "Oublier".
    // Pourquoi: l'action ne doit être possible que quand une MAC exploitable est connue.
    // Procédure détaillée:
    //   1) Ajouter manuellement un item avec adresse MAC en UserRole.
    //   2) Le sélectionner dans la liste.
    //   3) Vérifier l'activation de btnForget.
    qputenv("BT_MODE", "none");
    SettingsPage page;

    auto* item = new QListWidgetItem("📱 Test Device", page.ui->listDevices);
    item->setData(Qt::UserRole, QString("AA:BB:CC:DD:EE:99"));
    page.ui->listDevices->setCurrentItem(item);

    QVERIFY(page.ui->btnForget->isEnabled());
}

void SettingsPageUiTest::refreshPairedList_restoresSelection()
{
    // Objectif: tester la conservation de la sélection après rafraîchissement.
    // Pourquoi: l'utilisateur ne doit pas perdre son contexte à chaque update de liste.
    // Procédure détaillée:
    //   1) Simuler un device unique connecté.
    //   2) Sélectionner cet item puis relancer refreshPairedList().
    //   3) Vérifier que la même MAC reste sélectionnée ensuite.
    qputenv("BT_MODE", "single");
    SettingsPage page;

    page.refreshPairedList();
    QVERIFY(page.ui->listDevices->count() >= 1);
    auto* first = page.ui->listDevices->item(0);
    QVERIFY(first != nullptr);
    page.ui->listDevices->setCurrentItem(first);
    const QString selectedMac = first->data(Qt::UserRole).toString();

    page.refreshPairedList();

    QVERIFY(page.ui->listDevices->currentItem() != nullptr);
    QCOMPARE(page.ui->listDevices->currentItem()->data(Qt::UserRole).toString(), selectedMac);
}

void SettingsPageUiTest::refreshPairedList_multipleConnected_keepsOnlyNewcomer()
{
    // Objectif: valider la logique de déconnexion lorsqu'il existe plusieurs connexions actives.
    // Pourquoi: l'application veut conserver un seul appareil "nouveau" et libérer l'autre.
    // Procédure détaillée:
    //   1) Simuler deux appareils connectés (BT_MODE=multi).
    //   2) Rafraîchir la liste.
    //   3) Vérifier qu'une commande "disconnect <MAC>" est émise dans le log.
    qputenv("BT_MODE", "multi");
    QFile::remove(m_tempDir.path() + "/bt.log");

    SettingsPage page;
    page.refreshPairedList();

    const QString log = readLog();
    QVERIFY(log.contains("disconnect AA:BB:CC:DD:EE:01") || log.contains("disconnect AA:BB:CC:DD:EE:02"));
}

QTEST_MAIN(SettingsPageUiTest)
#include "tst_ui_settingspage.moc"
