#include "homeassistant.h"
#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QApplication>

HomeAssistant::HomeAssistant(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // On crée ou récupère un profil nommé "HomeAssistantProfile"
    QWebEngineProfile *profile = new QWebEngineProfile("HA_Profile", this);
    profile->setPersistentStoragePath(qApp->applicationDirPath() + "/web_data");
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    // On crée la page avec ce profil
    QWebEnginePage *page = new QWebEnginePage(profile, this);

    m_view = new QWebEngineView(this);
    m_view->setPage(page);

    // On force l'acceptation de tout le contenu local
    m_view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    m_view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    m_view->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    m_view->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    m_view->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    m_view->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    m_view->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    m_view->settings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true); // Plus fluide

    // Chargez votre URL (utilisez l'IP, évitez homeassistant.local si possible)
    m_view->setUrl(QUrl("http://192.168.1.158:8123"));

    connect(m_view, &QWebEngineView::loadFinished, [this]() {
        m_view->page()->runJavaScript(
            "document.querySelectorAll('input, textarea').forEach(el => {"
            "  el.addEventListener('focus', () => { console.log('SHOW_KEYBOARD'); });"
            "});"
            );
    });

    layout->addWidget(m_view);
}
