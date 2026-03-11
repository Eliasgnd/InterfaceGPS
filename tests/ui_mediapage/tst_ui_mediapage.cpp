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
    MediaPage page;

    QVERIFY(page.m_playerView != nullptr);
    QCOMPARE(page.m_playerView->source(), QUrl(QStringLiteral("qrc:/MediaPlayer.qml")));
}

void MediaPageUiTest::setCompactMode_updatesQmlRootProperty()
{
    MediaPage page;

    QTRY_VERIFY(page.m_playerView->rootObject() != nullptr);

    page.setCompactMode(true);
    QCOMPARE(page.m_playerView->rootObject()->property("isCompactMode").toBool(), true);

    page.setCompactMode(false);
    QCOMPARE(page.m_playerView->rootObject()->property("isCompactMode").toBool(), false);
}

QTEST_MAIN(MediaPageUiTest)
#include "tst_ui_mediapage.moc"
