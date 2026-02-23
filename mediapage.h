/**
 * @file mediapage.h
 * @brief Rôle architectural : Conteneur Widget de l'expérience multimédia QML.
 * @details Responsabilités : Héberger la vue QML du lecteur de musique, gérer son cycle de vie
 * et y injecter les dépendances C++ nécessaires (notamment la gestion du Bluetooth).
 * Dépendances principales : QWidget, QQuickWidget et BluetoothManager.
 */

#pragma once
#include <QWidget>
#include <QQuickWidget>

namespace Ui { class MediaPage; }

/**
 * @class MediaPage
 * @brief Page graphique dédiée à la gestion des médias (musique, Bluetooth).
 * * Cette classe agit comme un pont (Wrapper) : elle hérite de QWidget pour s'insérer
 * dans le layout classique du MainWindow, mais son contenu réel est propulsé par le moteur QML
 * pour offrir des animations et une UI plus moderne.
 */
class MediaPage : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructeur de la page média.
     * @param parent Widget parent (généralement MainWindow).
     */
    explicit MediaPage(QWidget* parent = nullptr);

    /**
     * @brief Destructeur. Libère l'interface générée par Qt Designer.
     */
    ~MediaPage();

    /**
     * @brief Adapte l'interface du lecteur multimédia en fonction de l'espace disponible.
     * Cette fonction est appelée par le MainWindow lorsque l'écran bascule en mode "Split-Screen"
     * (écran divisé), afin que la vue QML réduise la taille de ses boutons et de la jaquette.
     * @param compact True pour activer le mode réduit, False pour le mode plein écran.
     */
    void setCompactMode(bool compact);

private:
    // --- ATTRIBUTS ---
    Ui::MediaPage* ui;              ///< Interface utilisateur générée, fournissant le layout hôte.
    QQuickWidget* m_playerView;     ///< Conteneur intégrant la scène QML (MediaPlayer.qml).
};
