#include <QtTest>
#include <QQuickWidget>
#include <QQuickItem>

#define private public
#include "../../mediapage.h"
#undef private

class MediaPageUiTest : public QObject
{
    Q_OBJECT

private slots:
    void constructor_createsQmlPlayerView();
    void setCompactMode_updatesQmlRootProperty();
};

void MediaPageUiTest::constructor_createsQmlPlayerView()
{
    // Objectif: vérifier que MediaPage charge correctement son frontend QML.
    // Pourquoi: sans source QML valide, aucun contrôle média n'est affiché.
    // Procédure détaillée:
    //   1) Instancier MediaPage.
    //   2) Vérifier la création de m_playerView.
    //   3) Vérifier la source QML attendue (qrc:/MediaPlayer.qml).
    MediaPage page;

    QVERIFY(page.m_playerView != nullptr);
    QCOMPARE(page.m_playerView->source(), QUrl(QStringLiteral("qrc:/MediaPlayer.qml")));
}

void MediaPageUiTest::setCompactMode_updatesQmlRootProperty()
{
    // Objectif: valider le pont C++ -> QML pour la propriété d'affichage compact.
    // Pourquoi: ce flag pilote l'adaptation du layout lecteur selon le contexte écran.
    // Procédure détaillée:
    //   1) Attendre la disponibilité de rootObject (chargement asynchrone QML).
    //   2) Activer setCompactMode(true) puis vérifier isCompactMode=true.
    //   3) Désactiver setCompactMode(false) puis vérifier isCompactMode=false.
    MediaPage page;

    QTRY_VERIFY(page.m_playerView->rootObject() != nullptr);

    page.setCompactMode(true);
    QCOMPARE(page.m_playerView->rootObject()->property("isCompactMode").toBool(), true);

    page.setCompactMode(false);
    QCOMPARE(page.m_playerView->rootObject()->property("isCompactMode").toBool(), false);
}

QTEST_MAIN(MediaPageUiTest)
#include "tst_ui_mediapage.moc"
