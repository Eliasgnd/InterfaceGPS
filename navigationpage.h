/**
 * @file navigationpage.h
 * @brief Rôle architectural : Façade Widget de la navigation cartographique.
 * @details Responsabilités : Gérer la vue QML de la carte, orchestrer la recherche d'adresses,
 * afficher les suggestions dynamiques et lier le clavier virtuel personnalisé.
 * Dépendances principales : QWidget, QQuickWidget (pour l'intégration QML), Clavier, TelemetryData.
 */

#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>
#include <QQuickWidget>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QEvent>

namespace Ui { class NavigationPage; }
class TelemetryData;
class QCompleter;
class QStringListModel;
class QTimer;
class Clavier;

/**
 * @class NavigationPage
 * @brief Contrôleur de la page de navigation GPS.
 * * Héberge la carte (codée en QML) au sein de l'interface C++.
 * * Gère la saisie utilisateur via une barre de recherche interceptée par un clavier virtuel maison.
 * * Transfère les données de télémétrie du véhicule vers la carte visuelle.
 */
class NavigationPage : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructeur de la page de navigation.
     * @param parent Widget parent (généralement MainWindow).
     */
    explicit NavigationPage(QWidget* parent = nullptr);

    /**
     * @brief Destructeur.
     */
    ~NavigationPage();

    /**
     * @brief Connecte le bus de télémétrie à la carte.
     * @param t Pointeur vers les données en temps réel du véhicule (GPS, cap, vitesse).
     */
    void bindTelemetry(TelemetryData* t);

    /**
     * @brief Filtre d'événements global pour ce widget.
     * Utilisé ici pour intercepter les clics sur la barre de recherche afin d'ouvrir le clavier virtuel
     * au lieu du clavier système par défaut.
     * @param obj L'objet qui reçoit l'événement.
     * @param event L'événement (ex: clic de souris).
     * @return true si l'événement a été intercepté et traité, false sinon.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // --- SLOTS DE RÉCEPTION QML -> C++ ---

    /**
     * @brief Slot appelé quand la carte QML a calculé un itinéraire.
     * @param distance Chaîne formatée représentant la distance (ex: "15 km").
     * @param duration Chaîne formatée représentant le temps de trajet (ex: "20 min").
     */
    void onRouteInfoReceived(const QString& distance, const QString& duration);

    /**
     * @brief Slot appelé quand le code QML renvoie les suggestions d'autocomplétion.
     * @param jsonSuggestions Chaîne JSON contenant un tableau de suggestions d'adresses.
     */
    void onSuggestionsReceived(const QString& jsonSuggestions);

    // --- SLOTS DE RECHERCHE ---

    /**
     * @brief Appliqué quand l'utilisateur sélectionne une adresse dans la liste déroulante.
     * @param suggestion Le texte complet de l'adresse choisie.
     */
    void onSuggestionChosen(const QString& suggestion);

    /**
     * @brief Déclenche une requête de suggestions d'adresses vers l'API cartographique.
     * Appelée par le timer de "debounce" pour ne pas spammer l'API à chaque frappe de touche.
     */
    void triggerSuggestionsSearch();

private:
    // --- MÉTHODES INTERNES ---

    /**
     * @brief Envoie l'ordre à la carte QML de calculer un itinéraire vers cette destination.
     * @param destination L'adresse ou le lieu recherché.
     */
    void requestRouteForText(const QString& destination);

    /**
     * @brief Instancie et affiche le clavier virtuel personnalisé de l'application.
     */
    void openVirtualKeyboard();

    // --- ATTRIBUTS ---
    Ui::NavigationPage* ui;                    ///< Interface utilisateur générée.
    TelemetryData* m_t = nullptr;              ///< Référence aux données du véhicule.
    QQuickWidget* m_mapView = nullptr;         ///< Conteneur intégrant le code QML de la carte.

    // Autocomplétion
    QCompleter* m_searchCompleter = nullptr;       ///< Moteur d'autocomplétion Qt.
    QStringListModel* m_suggestionsModel = nullptr; ///< Modèle de données stockant les adresses suggérées.
    QTimer* m_suggestionDebounceTimer = nullptr;    ///< Timer pour différer les requêtes réseau (anti-spam).
    bool m_ignoreTextUpdate = false;               ///< Flag pour éviter les boucles infinies lors de la sélection d'une suggestion.

    Clavier* m_currentClavier = nullptr;           ///< Pointeur vers l'instance active du clavier virtuel.
};

#endif // NAVIGATIONPAGE_H
