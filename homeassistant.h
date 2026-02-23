/**
 * @file homeassistant.h
 * @brief Rôle architectural : Intégration embarquée de l'interface Home Assistant.
 * @details Responsabilités : Encapsuler la vue web (QWebEngineView) et relayer
 * les demandes d'ouverture de clavier virtuel (interceptées via JavaScript) vers l'application Qt.
 * Dépendances principales : QWebEngineView, QWebEnginePage et signalisation interne UI.
 */

#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>

/**
 * @class HAPage
 * @brief Surcharge de la page web pour intercepter les événements de la console.
 * * Cette classe permet d'écouter les "console.log" émis par le code JavaScript
 * injecté dans la page Home Assistant, créant ainsi un canal de communication
 * du Web vers le C++.
 */
class HAPage : public QWebEnginePage {
    Q_OBJECT
public:
    /**
     * @brief Constructeur de la page web personnalisée.
     * @param profile Profil WebEngine gérant les cookies et le cache.
     * @param parent Objet parent pour la gestion mémoire.
     */
    explicit HAPage(QWebEngineProfile* profile, QObject* parent = nullptr);

protected:
    /**
     * @brief Intercepte les messages de la console JavaScript.
     * Si le message correspond à un mot-clé précis (ex: "SHOW_KEYBOARD"),
     * un signal C++ est émis.
     */
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) override;

signals:
    /**
     * @brief Émis lorsque le code JavaScript détecte un focus sur un champ de saisie.
     */
    void showKeyboardRequested();
};

/**
 * @class HomeAssistant
 * @brief Page principale affichant le dashboard domotique.
 * * Gère l'initialisation du moteur de rendu Chromium (WebEngine).
 * * Assure l'apparition du clavier tactile maison par-dessus la vue web.
 */
class HomeAssistant : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructeur de l'interface Home Assistant.
     * @param parent Widget parent.
     */
    explicit HomeAssistant(QWidget* parent = nullptr);

private slots:
    /**
     * @brief Ouvre le clavier virtuel modale.
     * Déclenché par le signal showKeyboardRequested de la HAPage.
     * Récupère la saisie et l'injecte dans la page web via JavaScript.
     */
    void openKeyboard();

private:
    // --- ATTRIBUTS ---
    QWebEngineView* m_view; ///< Le composant graphique affichant la page web.
};

#endif // HOMEASSISTANT_H
