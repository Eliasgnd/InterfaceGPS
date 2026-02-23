/**
 * @file settingspage.h
 * @brief Rôle architectural : Page de configuration système et Bluetooth utilisateur.
 * @details Responsabilités : Piloter les préférences de l'UI et administrer la liste
 * des périphériques Bluetooth appairés (smartphones).
 * Dépendances principales : QWidget, QProcess (pour bluetoothctl), QTimer et UI générée.
 */

#pragma once
#include <QWidget>
#include <QBluetoothLocalDevice>
#include <QTimer>
#include <QSet>

namespace Ui { class SettingsPage; }
class TelemetryData;

/**
 * @class SettingsPage
 * @brief Interface graphique de gestion des paramètres et des connexions sans fil.
 * * Permet de rendre le véhicule visible pour l'appairage de nouveaux téléphones.
 * * Affiche la liste des appareils connus et leur état de connexion en temps réel.
 * * Permet "d'oublier" (supprimer) un appareil existant.
 */
class SettingsPage : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructeur de la page des paramètres.
     * @param parent Widget parent (généralement MainWindow).
     */
    explicit SettingsPage(QWidget* parent = nullptr);

    /**
     * @brief Destructeur. Libère l'interface générée par Qt Designer.
     */
    ~SettingsPage();

private slots:
    // --- SLOTS UTILISATEUR (UI) ---
    void onVisibleClicked(); ///< Déclenchée lors du clic sur le bouton "Rendre Visible".
    void onForgetClicked();  ///< Déclenchée lors du clic sur le bouton "Oublier l'appareil".

    // --- SLOTS SYSTÈME ---
    /**
     * @brief Capture et journalise les erreurs de l'adaptateur Bluetooth local.
     * @param error Le code d'erreur émis par le module QtBluetooth.
     */
    void errorOccurred(QBluetoothLocalDevice::Error error);

    /**
     * @brief Coupe la visibilité Bluetooth du véhicule à la fin du timer de sécurité.
     */
    void stopDiscovery();

    /**
     * @brief Interroge le système (bluetoothctl) pour mettre à jour la liste des appareils.
     * Appelée périodiquement (polling) par m_pollTimer.
     */
    void refreshPairedList();

private:
    // --- MÉTHODES INTERNES ---

    /**
     * @brief Active ou désactive la découvrabilité (Mode Appairage) de l'adaptateur Bluetooth.
     * @param enable True pour rendre le véhicule visible par les smartphones, False pour le cacher.
     */
    void setDiscoverable(bool enable);

    /**
     * @brief Affiche une boîte de dialogue d'information (Popup) qui se ferme toute seule.
     * @param title Titre de la fenêtre.
     * @param text Contenu du message.
     * @param timeoutMs Durée d'affichage en millisecondes avant fermeture automatique.
     */
    void showAutoClosingMessage(const QString &title, const QString &text, int timeoutMs);

    // --- ATTRIBUTS ---
    Ui::SettingsPage* ui;                    ///< Interface utilisateur générée.

    QBluetoothLocalDevice *m_localDevice;    ///< Contrôleur de l'adaptateur Bluetooth physique de la machine.
    QTimer *m_discoveryTimer;                ///< Timer de sécurité limitant la durée du mode "Visible".
    QTimer *m_pollTimer;                     ///< Timer de rafraîchissement de la liste des périphériques (Polling).

    QSet<QString> m_knownMacs;               ///< Registre mémoire des adresses MAC déjà approuvées ("trust").
    QString m_lastActiveMac;                 ///< Adresse MAC du dernier périphérique connecté.
};
