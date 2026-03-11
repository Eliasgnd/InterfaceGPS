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
    // Objectif: valider l'état initial de sécurité de l'écran Bluetooth.
    // Pourquoi: sans sélection d'appareil, l'action "Forget" doit rester impossible.
    // Procédure détaillée:
    //   1) Instancier SettingsPage.
    //   2) Vérifier que btnForget est désactivé à froid.
    SettingsPage page;

    QVERIFY(!page.ui->btnForget->isEnabled());
}

void SettingsPageUiTest::setDiscoverable_true_updatesButtonAndTimer()
{
    // Objectif: vérifier la transition vers l'état discoverable.
    // Pourquoi: l'utilisateur doit voir clairement que l'appairage est ouvert temporairement.
    // Procédure détaillée:
    //   1) Appeler setDiscoverable(true).
    //   2) Vérifier bouton coché + texte "Visible (120s max)...".
    //   3) Vérifier que le timer de découverte est actif.
    SettingsPage page;

    page.setDiscoverable(true);

    QVERIFY(page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Visible (120s max)..."));
    QVERIFY(page.m_discoveryTimer->isActive());
}

void SettingsPageUiTest::setDiscoverable_false_updatesButtonAndTimer()
{
    // Objectif: vérifier la sortie propre du mode discoverable.
    // Pourquoi: l'arrêt doit remettre un état UI cohérent et arrêter le timer associé.
    // Procédure détaillée:
    //   1) Activer discoverable puis le désactiver.
    //   2) Vérifier bouton non coché + texte par défaut.
    //   3) Vérifier arrêt du timer.
    SettingsPage page;
    page.setDiscoverable(true);

    page.setDiscoverable(false);

    QVERIFY(!page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Rendre Visible (Appairage)"));
    QVERIFY(!page.m_discoveryTimer->isActive());
}

void SettingsPageUiTest::stopDiscovery_alwaysResetsVisibleState()
{
    // Objectif: valider l'action explicite stopDiscovery indépendamment de l'état courant.
    // Pourquoi: cette action est utilisée comme garde-fou pour forcer la fermeture de visibilité.
    // Procédure détaillée:
    //   1) Mettre la page en discoverable.
    //   2) Appeler stopDiscovery().
    //   3) Vérifier le retour complet à l'état non visible (toggle + libellé).
    SettingsPage page;
    page.setDiscoverable(true);
    QVERIFY(page.ui->btnVisible->isChecked());

    page.stopDiscovery();

    QVERIFY(!page.ui->btnVisible->isChecked());
    QCOMPARE(page.ui->btnVisible->text(), QString("Rendre Visible (Appairage)"));
}

void SettingsPageUiTest::refreshPairedList_withNoSelection_keepsForgetDisabled()
{
    // Objectif: vérifier le comportement de refresh sans appareil sélectionné.
    // Pourquoi: l'écran doit rester sûr (pas de suppression accidentelle) tout en affichant des entrées.
    // Procédure détaillée:
    //   1) Appeler refreshPairedList().
    //   2) Vérifier que btnForget reste désactivé.
    //   3) Vérifier que la liste contient au moins une ligne (placeholder ou device).
    SettingsPage page;

    page.refreshPairedList();

    QVERIFY(!page.ui->btnForget->isEnabled());
    QVERIFY(page.ui->listDevices->count() >= 1);
}

QTEST_MAIN(SettingsPageUiTest)
#include "tst_ui_settingspage.moc"
