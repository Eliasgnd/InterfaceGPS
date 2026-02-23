/**
 * @file homeassistant.cpp
 * @brief Implémentation du module Home Assistant dans le tableau de bord.
 * @details Responsabilités : Configurer la page web (cache, accélération matérielle),
 * injecter des écouteurs d'événements JavaScript (Event Listeners) et piloter
 * le clavier natif pour interagir avec les champs de texte HTML.
 * Dépendances principales : Qt WebEngine, clavier virtuel (Clavier) et logique de page.
 */

#include "homeassistant.h"
#include "clavier.h"
#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QApplication>
#include <QTimer>

HAPage::HAPage(QWebEngineProfile* profile, QObject* parent) : QWebEnginePage(profile, parent) {}

void HAPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) {
    // Écoute spécifique du message "SHOW_KEYBOARD" envoyé par notre script injecté
    if (message == "SHOW_KEYBOARD") {
        emit showKeyboardRequested();
    }

    // Appel de la méthode parente pour conserver le comportement standard
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}

HomeAssistant::HomeAssistant(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // --- CONFIGURATION DU PROFIL WEB (Persistance) ---
    // Permet de conserver la session de connexion à Home Assistant entre les redémarrages.
    QWebEngineProfile *profile = new QWebEngineProfile("HA_Profile", this);
    profile->setPersistentStoragePath(qApp->applicationDirPath() + "/web_data");
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    // Instanciation de notre page personnalisée pour l'interception des logs
    HAPage *page = new HAPage(profile, this);

    // Liaison du signal de la page au déclenchement du clavier C++
    connect(page, &HAPage::showKeyboardRequested, this, &HomeAssistant::openKeyboard);

    m_view = new QWebEngineView(this);
    m_view->setPage(page);

    // --- CONFIGURATION DES PERFORMANCES WEBENGINE ---
    QWebEngineSettings *s = m_view->settings();
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true); // Requis si HA n'est pas en HTTPS localement
    s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);  // Boost l'affichage des graphiques
    s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);       // Rendu fluide du défilement

    // URL locale du serveur Home Assistant
    m_view->setUrl(QUrl("http://192.168.1.158:8123"));

    // --- INJECTION JAVASCRIPT ---
    // Une fois la page chargée, on injecte un script qui écoute les clics sur les champs de texte
    connect(m_view, &QWebEngineView::loadFinished, [this]() {
        m_view->page()->runJavaScript(
            "window._ha_last_input = null;"
            "window.addEventListener('focusin', (e) => {"
            "  let active = e.composedPath()[0];"
            "  if (active && (active.tagName === 'INPUT' || active.tagName === 'TEXTAREA')) {"
            "    window._ha_last_input = active;"
            "    console.log('SHOW_KEYBOARD');" // Ce message est intercepté par HAPage
            "  }"
            "});"
            );
    });

    layout->addWidget(m_view);
}

void HomeAssistant::openKeyboard() {
    // Sécurité "Anti-Rebond" (Debounce) pour éviter d'ouvrir 10 claviers
    // si le JavaScript s'emballe et envoie plusieurs événements d'un coup.
    static bool isKeyboardOpen = false;
    if (isKeyboardOpen) return;

    isKeyboardOpen = true;

    // Instanciation modale du clavier virtuel
    Clavier clavier(this);

    // Si l'utilisateur valide sa saisie
    if (clavier.exec() == QDialog::Accepted) {
        QString res = clavier.getText();

        // Échappement des guillemets pour ne pas casser la syntaxe JavaScript lors de l'injection
        res.replace("'", "\\'");
        res.replace("\"", "\\\"");

        // Script JavaScript pour injecter le texte dans le champ web ciblé
        // et simuler les événements "input" et "change" pour que le framework web (ex: Vue/React) prenne en compte la valeur.
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
        // Si l'utilisateur annule (ferme le clavier sans valider), on retire le focus HTML
        m_view->page()->runJavaScript(
            "if (window._ha_last_input) {"
            "  window._ha_last_input.blur();"
            "}"
            );
    }

    // Réinitialisation du verrouillage Anti-Rebond après un court délai
    QTimer::singleShot(400, []() {
        isKeyboardOpen = false;
    });
}
