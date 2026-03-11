#include <QtTest>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEnginePage>

#define private public
#include "../../homeassistant.h"
#undef private

class TestableHAPage : public HAPage
{
    Q_OBJECT
public:
    explicit TestableHAPage(QWebEngineProfile* profile)
        : HAPage(profile) {}

    void emitConsole(JavaScriptConsoleMessageLevel level,
                     const QString& message,
                     int lineNumber = 1,
                     const QString& sourceId = QStringLiteral("test"))
    {
        javaScriptConsoleMessage(level, message, lineNumber, sourceId);
    }
};

class HomeAssistantUiTest : public QObject
{
    Q_OBJECT

private slots:
    void haPage_consoleShowKeyboard_emitsSignal();
    void haPage_otherConsoleMessages_doNotEmitSignal();
    void homeAssistant_constructor_configuresViewAndPage();
    void homeAssistant_constructor_appliesRequiredWebEngineSettings();
};

void HomeAssistantUiTest::haPage_consoleShowKeyboard_emitsSignal()
{
    QWebEngineProfile profile;
    TestableHAPage page(&profile);
    QSignalSpy spy(&page, &HAPage::showKeyboardRequested);

    page.emitConsole(QWebEnginePage::InfoMessageLevel, QStringLiteral("SHOW_KEYBOARD"));

    QCOMPARE(spy.count(), 1);
}

void HomeAssistantUiTest::haPage_otherConsoleMessages_doNotEmitSignal()
{
    QWebEngineProfile profile;
    TestableHAPage page(&profile);
    QSignalSpy spy(&page, &HAPage::showKeyboardRequested);

    page.emitConsole(QWebEnginePage::InfoMessageLevel, QStringLiteral("OTHER_MESSAGE"));

    QCOMPARE(spy.count(), 0);
}

void HomeAssistantUiTest::homeAssistant_constructor_configuresViewAndPage()
{
    HomeAssistant page;

    QVERIFY(page.m_view != nullptr);
    QVERIFY(page.m_view->page() != nullptr);
    QVERIFY(qobject_cast<HAPage*>(page.m_view->page()) != nullptr);
    QCOMPARE(page.m_view->url(), QUrl(QStringLiteral("http://192.168.1.158:8123")));
}

void HomeAssistantUiTest::homeAssistant_constructor_appliesRequiredWebEngineSettings()
{
    HomeAssistant page;
    QWebEngineSettings* s = page.m_view->settings();

    QVERIFY(s->testAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls));
    QVERIFY(s->testAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls));
    QVERIFY(s->testAttribute(QWebEngineSettings::AllowRunningInsecureContent));
    QVERIFY(s->testAttribute(QWebEngineSettings::PlaybackRequiresUserGesture) == false);
    QVERIFY(s->testAttribute(QWebEngineSettings::LocalStorageEnabled));
    QVERIFY(s->testAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled));
    QVERIFY(s->testAttribute(QWebEngineSettings::WebGLEnabled));
    QVERIFY(s->testAttribute(QWebEngineSettings::ScrollAnimatorEnabled));
}

QTEST_MAIN(HomeAssistantUiTest)
#include "tst_ui_homeassistant.moc"
