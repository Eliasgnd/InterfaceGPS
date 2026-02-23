/**
 * @file camerapage.h
 * @brief Rôle architectural : Page UI dédiée à l'affichage du flux caméra embarqué (ex: vue recul ou Bird-eye).
 * @details Responsabilités : Gérer le cycle de vie de l'écoute UDP (ouverture/fermeture du port)
 * et convertir les datagrammes réseau en images affichables sur l'interface graphique.
 * Dépendances principales : QWidget, QUdpSocket (Qt Network), QLabel et UI générée.
 */

#ifndef CAMERAPAGE_H
#define CAMERAPAGE_H

#include <QWidget>
#include <QUdpSocket>
#include <QLabel>

namespace Ui {
class CameraPage;
}

/**
 * @class CameraPage
 * @brief Contrôleur de la vue caméra.
 * * Cette classe écoute sur un port UDP spécifique (4444) pour recevoir des flux vidéo
 * sous forme de trames JPEG successives (Motion JPEG sur UDP).
 * * L'écoute réseau est dynamiquement activée ou désactivée par le MainWindow
 * selon que la page est visible ou non, afin de préserver les ressources CPU/Réseau.
 */
class CameraPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur de la page Caméra.
     * Initialise l'interface et prépare le socket UDP sans pour autant l'ouvrir.
     * @param parent Widget parent (généralement MainWindow).
     */
    explicit CameraPage(QWidget *parent = nullptr);

    /**
     * @brief Destructeur. Libère l'interface générée.
     */
    ~CameraPage();

public slots:
    // --- SLOTS DE CONTRÔLE DU FLUX ---

    /**
     * @brief Démarre l'écoute du flux vidéo entrant.
     * Ouvre le port UDP 4444 et se met en attente de datagrammes.
     * Appelée par le MainWindow lorsque l'utilisateur affiche cette page.
     */
    void startStream();

    /**
     * @brief Arrête l'écoute du flux vidéo entrant.
     * Ferme le port UDP et efface la dernière image affichée.
     * Appelée par le MainWindow lorsque l'utilisateur quitte cette page (ex: passage au mode navigation).
     */
    void stopStream();

private slots:
    // --- SLOTS DE TRAITEMENT RÉSEAU ---

    /**
     * @brief Intercepte et décode les paquets réseau entrants.
     * Déclenchée automatiquement dès que le socket UDP reçoit des données (signal readyRead).
     * Tente de convertir le payload binaire en image JPEG pour l'afficher sur le label.
     */
    void processPendingDatagrams();

private:
    // --- ATTRIBUTS ---
    Ui::CameraPage *ui;         ///< Interface utilisateur générée par Qt Designer.
    QUdpSocket *udpSocket;      ///< Socket réseau dédié à la réception du flux vidéo local.
    QLabel *videoLabel;         ///< Référence directe au widget QLabel chargé d'afficher les frames vidéo.
};

#endif // CAMERAPAGE_H
