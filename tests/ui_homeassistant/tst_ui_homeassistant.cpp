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

// Suite de tests UI Home Assistant: chaque test explique le comportement attendu
// en termes simples pour faciliter la relecture par des personnes non expertes Qt/WebEngine.
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
    // Résultat attendu en clair: le message console "SHOW_KEYBOARD" agit comme un bouton virtuel
    // qui demande explicitement l'ouverture du clavier côté interface Qt.
    // Objectif: vérifier l'intégration JS->Qt pour l'ouverture clavier.
    // Pourquoi: Home Assistant déclenche le clavier via un message console dédié.
    // Procédure détaillée:
    //   1) Créer une HAPage testable et brancher QSignalSpy sur showKeyboardRequested.
    //   2) Simuler un message console SHOW_KEYBOARD.
    //   3) Vérifier qu'un unique signal est émis.
    QWebEngineProfile profile;
    TestableHAPage page(&profile);
    QSignalSpy spy(&page, &HAPage::showKeyboardRequested);

    page.emitConsole(QWebEnginePage::InfoMessageLevel, QStringLiteral("SHOW_KEYBOARD"));

    QCOMPARE(spy.count(), 1);
}

void HomeAssistantUiTest::haPage_otherConsoleMessages_doNotEmitSignal()
{
    // Objectif: garantir qu'aucun faux déclenchement clavier ne survient.
    // Pourquoi: un filtrage permissif créerait des ouvertures clavier inattendues.
    // Procédure détaillée:
    //   1) Garder le même setup avec QSignalSpy.
    //   2) Simuler un message console sans mot-clé.
    //   3) Vérifier zéro émission de showKeyboardRequested.
    QWebEngineProfile profile;
    TestableHAPage page(&profile);
    QSignalSpy spy(&page, &HAPage::showKeyboardRequested);

    page.emitConsole(QWebEnginePage::InfoMessageLevel, QStringLiteral("OTHER_MESSAGE"));

    QCOMPARE(spy.count(), 0);
}

void HomeAssistantUiTest::homeAssistant_constructor_configuresViewAndPage()
{
    // Objectif: valider l'assemblage de base du composant HomeAssistant.
    // Pourquoi: une page non câblée ou une mauvaise URL rend l'écran inutilisable.
    // Procédure détaillée:
    //   1) Instancier HomeAssistant.
    //   2) Vérifier la création de m_view et de sa page interne.
    //   3) Vérifier le type HAPage et l'URL de destination configurée.
    HomeAssistant page;

    QVERIFY(page.m_view != nullptr);
    QVERIFY(page.m_view->page() != nullptr);
    QVERIFY(qobject_cast<HAPage*>(page.m_view->page()) != nullptr);
    QCOMPARE(page.m_view->url(), QUrl(QStringLiteral("http://192.168.1.158:8123")));
}

void HomeAssistantUiTest::homeAssistant_constructor_appliesRequiredWebEngineSettings()
{
    // Résultat attendu en clair: la page est configurée en "mode compatible HA"
    // (contenu mixte, stockage local, rendu accéléré, lecture média sans geste utilisateur).
    // Objectif: vérifier la configuration WebEngine nécessaire au bon fonctionnement HA.
    // Pourquoi: certains widgets/contenus nécessitent accès local/remote, WebGL et playback sans geste.
    // Procédure détaillée:
    //   1) Récupérer QWebEngineSettings depuis la vue.
    //   2) Lire chaque attribut critique.
    //   3) Vérifier qu'ils correspondent au profil d'exécution attendu.
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
