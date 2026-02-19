#include "homeassistant.h"
#include "clavier.h" // <--- INDISPENSABLE : on inclut ton clavier
#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QApplication>
#include <QTimer>

// --- Implémentation de HAPage ---
HAPage::HAPage(QWebEngineProfile* profile, QObject* parent) : QWebEnginePage(profile, parent) {}

void HAPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) {
    if (message == "SHOW_KEYBOARD") {
        emit showKeyboardRequested(); // Alerte le C++ !
    }
    // On laisse le reste des messages console s'afficher (optionnel)
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}

// --- Implémentation de HomeAssistant ---
HomeAssistant::HomeAssistant(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWebEngineProfile *profile = new QWebEngineProfile("HA_Profile", this);
    profile->setPersistentStoragePath(qApp->applicationDirPath() + "/web_data");
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    // On utilise HAPage au lieu d'un QWebEnginePage normal
    HAPage *page = new HAPage(profile, this);

    // On connecte le signal du JavaScript à l'ouverture de notre clavier !
    connect(page, &HAPage::showKeyboardRequested, this, &HomeAssistant::openKeyboard);

    m_view = new QWebEngineView(this);
    m_view->setPage(page);

    // Optimisations de vitesse
    QWebEngineSettings *s = m_view->settings();
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);

    m_view->setUrl(QUrl("http://192.168.1.158:8123"));

    // Script JavaScript ultra-robuste pour intercepter les clics sur Home Assistant
    connect(m_view, &QWebEngineView::loadFinished, [this]() {
        m_view->page()->runJavaScript(
            "window._ha_last_input = null;"
            "window.addEventListener('focusin', (e) => {"
            "  let active = e.composedPath()[0];" // Trouve l'input même caché dans un Shadow DOM
            "  if (active && (active.tagName === 'INPUT' || active.tagName === 'TEXTAREA')) {"
            "    window._ha_last_input = active;"
            "    console.log('SHOW_KEYBOARD');"
            "  }"
            "});"
            );
    });

    layout->addWidget(m_view);
}

// --- La magie : lier ton clavier C++ au site web ---
void HomeAssistant::openKeyboard() {
    static bool isKeyboardOpen = false;
    if (isKeyboardOpen) return;

    isKeyboardOpen = true;

    Clavier clavier(this);

    if (clavier.exec() == QDialog::Accepted) {
        QString res = clavier.getText();
        res.replace("'", "\\'");
        res.replace("\"", "\\\"");

        QString js = QString(
                         "if (window._ha_last_input) {"
                         "  window._ha_last_input.value = '%1';"
                         "  window._ha_last_input.dispatchEvent(new Event('input', { bubbles: true }));"
                         "  window._ha_last_input.dispatchEvent(new Event('change', { bubbles: true }));"
                         "  window._ha_last_input.blur();"
                         "}"
                         ).arg(res);

        m_view->page()->runJavaScript(js);
    } else {
        m_view->page()->runJavaScript(
            "if (window._ha_last_input) {"
            "  window._ha_last_input.blur();"
            "}"
            );
    }

    // LA SOLUTION : On utilise un Timer pour attendre 400 millisecondes
    // avant d'autoriser le clavier à s'ouvrir de nouveau.
    // Cela bloque les "re-focus" agressifs de Home Assistant.
    QTimer::singleShot(400, []() {
        isKeyboardOpen = false;
    });
}
