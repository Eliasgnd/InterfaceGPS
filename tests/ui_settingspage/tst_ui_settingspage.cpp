#include <QtTest>
#include <QPushButton>
#include <QListWidget>

#include "ui_settingspage.h"

#define private public
#include "../../settingspage.h"
#undef private

class SettingsPageUiTest : public QObject
{
    Q_OBJECT

private slots:
    void constructor_initialState_forgetButtonDisabled();
    void setDiscoverable_true_updatesButtonAndTimer();
    void setDiscoverable_false_updatesButtonAndTimer();
    void stopDiscovery_alwaysResetsVisibleState();
    void refreshPairedList_withNoSelection_keepsForgetDisabled();
};

void SettingsPageUiTest::constructor_initialState_forgetButtonDisabled()
{
    SettingsPage page;

    QVERIFY(!page.ui->btnForget->isEnabled());
}

void SettingsPageUiTest::setDiscoverable_true_updatesButtonAndTimer()
{
    SettingsPage page;

    page.setDiscoverable(true);

    QVERIFY(page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Visible (120s max)..."));
    QVERIFY(page.m_discoveryTimer->isActive());
}

void SettingsPageUiTest::setDiscoverable_false_updatesButtonAndTimer()
{
    SettingsPage page;
    page.setDiscoverable(true);

    page.setDiscoverable(false);

    QVERIFY(!page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Rendre Visible (Appairage)"));
    QVERIFY(!page.m_discoveryTimer->isActive());
}

void SettingsPageUiTest::stopDiscovery_alwaysResetsVisibleState()
{
    SettingsPage page;
    page.setDiscoverable(true);
    QVERIFY(page.ui->btnVisible->isChecked());

    page.stopDiscovery();

    QVERIFY(!page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Rendre Visible (Appairage)"));
}

void SettingsPageUiTest::refreshPairedList_withNoSelection_keepsForgetDisabled()
{
    SettingsPage page;

    page.refreshPairedList();

    QVERIFY(!page.ui->btnForget->isEnabled());
    QVERIFY(page.ui->listDevices->count() >= 1);
}

QTEST_MAIN(SettingsPageUiTest)
#include "tst_ui_settingspage.moc"
