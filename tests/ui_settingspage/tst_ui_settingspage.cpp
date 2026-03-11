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
    qputenv("BT_MODE", "none");
    SettingsPage page;

    QVERIFY(!page.ui->btnForget->isEnabled());
}

void SettingsPageUiTest::setDiscoverable_true_updatesButtonAndTimer()
{
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
    qputenv("BT_MODE", "none");
    SettingsPage page;

    page.refreshPairedList();

    QVERIFY(!page.ui->btnForget->isEnabled());
    QVERIFY(page.ui->listDevices->count() >= 1);
}

void SettingsPageUiTest::listSelection_withValidMac_enablesForgetButton()
{
    qputenv("BT_MODE", "none");
    SettingsPage page;

    auto* item = new QListWidgetItem("📱 Test Device", page.ui->listDevices);
    item->setData(Qt::UserRole, QString("AA:BB:CC:DD:EE:99"));
    page.ui->listDevices->setCurrentItem(item);

    QVERIFY(page.ui->btnForget->isEnabled());
}

void SettingsPageUiTest::refreshPairedList_restoresSelection()
{
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
    qputenv("BT_MODE", "multi");
    QFile::remove(m_tempDir.path() + "/bt.log");

    SettingsPage page;
    page.refreshPairedList();

    const QString log = readLog();
    QVERIFY(log.contains("disconnect AA:BB:CC:DD:EE:01") || log.contains("disconnect AA:BB:CC:DD:EE:02"));
}

QTEST_MAIN(SettingsPageUiTest)
#include "tst_ui_settingspage.moc"
