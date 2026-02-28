/**
 * @file mainwindow.h
 * @brief Rôle architectural : Orchestrateur principal des pages du tableau de bord.
 * @details Responsabilités : Gérer la navigation entre les différents modules,
 * sauvegarder l'état de l'interface (persistance) et gérer le mode "écran partagé" (Split-Screen).
 * Dépendances principales : QMainWindow, pages métiers (Navigation, Media, etc.), TelemetryData et QSettings.
 */

#pragma once
#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>

class TelemetryData;
class NavigationPage;
class CameraPage;
class SettingsPage;
class MediaPage;
class HomeAssistant;

#ifdef ENABLE_ANDROID_AUTO
class AndroidAutoPage;
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Fenêtre principale agissant comme conteneur parent de toutes les vues de l'application.
 * * Cette classe instancie toutes les sous-pages au démarrage et les garde en mémoire.
 * Au lieu de détruire/recréer les pages lors de la navigation, elle se contente de
 * masquer ou d'afficher les widgets concernés, ce qui garantit une interface réactive.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructeur de la fenêtre principale.
     * @param telemetry Pointeur vers le modèle de données partagé (capteurs, GPS).
     * @param parent Widget parent (généralement nullptr car c'est la fenêtre racine).
     */
    explicit MainWindow(TelemetryData* telemetry, QWidget* parent = nullptr);

    /**
     * @brief Destructeur. Libère l'interface générée par Qt Designer.
     */
    ~MainWindow();

private slots:
    // --- SLOTS DE NAVIGATION ---
    // Méthodes appelées lors du clic sur les boutons de la barre de navigation.

    void goNav();              ///< Affiche la page de Navigation (et la définit comme application gauche en mode split).
    void goCam();              ///< Affiche la page Caméra (mode plein écran uniquement).
    void goMedia();            ///< Affiche la page Média (et la définit comme application droite en mode split).
    void goSettings();         ///< Affiche la page Paramètres.
    void goHomeAssistant();    ///< Affiche la page Domotique.
    void goAndroidAuto();      ///< Affiche la page Android Auto.
    void goSplit();            ///< Active le mode écran partagé (Split-Screen) avec les dernières applications utilisées.

    // --- SLOTS UTILITAIRES ---

    /**
     * @brief Met à jour le bandeau d'alerte global (Top Bar).
     * S'active automatiquement via les signaux de TelemetryData.
     */
    void updateTopBarAndAlert();

    /**
     * @brief Alterne entre le mode plein écran de l'application courante et le mode Split-Screen.
     */
    void toggleSplitAndHome();

private:
    Ui::MainWindow* ui;              ///< Pointeur vers l'interface générée par Qt Designer (.ui).
    TelemetryData* m_t = nullptr;    ///< Référence partagée aux données du véhicule.

    // --- INSTANCES DES PAGES ---
    NavigationPage* m_nav = nullptr;
    CameraPage* m_cam = nullptr;
    MediaPage* m_media = nullptr;
    SettingsPage* m_settings = nullptr;
    HomeAssistant* m_ha = nullptr;

#ifdef ENABLE_ANDROID_AUTO
    AndroidAutoPage* m_aa = nullptr; ///< Instance de la page Android Auto
#endif

    // --- GESTION DE L'AFFICHAGE ---
    QHBoxLayout* m_mainLayout = nullptr; ///< Layout principal contenant toutes les pages.
    QPushButton* m_btnSplit = nullptr;   ///< Bouton dynamique permettant d'activer le mode Split-Screen.
    bool m_isSplitMode = false;          ///< Indique si l'interface est actuellement en écran divisé.

    // --- PERSISTANCE D'ÉTAT ---
    QWidget* m_lastLeftApp = nullptr;    ///< Pointeur vers la dernière vue affichée à gauche.
    QWidget* m_lastRightApp = nullptr;   ///< Pointeur vers la dernière vue affichée à droite.

    /**
     * @brief Gère l'affichage, le masquage et les proportions des pages dans le layout principal.
     * @param page1 La page principale à afficher.
     * @param page2 (Optionnel) La seconde page à afficher pour activer le mode Split-Screen.
     */
    void displayPages(QWidget* page1, QWidget* page2 = nullptr);

    /**
     * @brief Sauvegarde la configuration actuelle de l'écran partagé sur le disque (via QSettings).
     */
    void saveSplitState();

    /**
     * @brief Convertit un pointeur de widget en chaîne de caractères (pour la sauvegarde dans QSettings).
     */
    QString widgetToString(QWidget* w);

    /**
     * @brief Retrouve le pointeur du widget correspondant à une chaîne de caractères sauvegardée.
     */
    QWidget* stringToWidget(const QString& name);
};
