/**
 * @file bluetoothmanager.h
 * @brief Rôle architectural : Interface de contrôle média Bluetooth via MPRIS/DBus.
 * @details Responsabilités : Exposer les métadonnées (titre, artiste), l'état de lecture
 * et les commandes (Play, Pause, Next) au reste de l'application (notamment à QML).
 * Dépendances principales : Qt DBus et les services org.mpris.MediaPlayer2.*.
 */

#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <QObject>
#include <QtDBus/QtDBus>

/**
 * @class BluetoothManager
 * @brief Gestionnaire de communication avec les lecteurs multimédias du système d'exploitation.
 * * Cette classe écoute le bus de session DBus pour détecter la présence de lecteurs
 * compatibles MPRIS (ex: Spotify, lecteur Bluetooth du téléphone connecté, VLC).
 * * Elle expose ensuite ces données sous forme de propriétés Qt (Q_PROPERTY) pour
 * permettre une intégration fluide et réactive avec l'interface graphique (QML/C++).
 */
class BluetoothManager : public QObject {
    Q_OBJECT

    // --- PROPRIÉTÉS EXPOSÉES À L'INTERFACE ---
    Q_PROPERTY(QString title READ title NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metadataChanged)
    Q_PROPERTY(QString album READ album NOTIFY metadataChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY statusChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY metadataChanged)

public:
    /**
     * @brief Constructeur du gestionnaire Bluetooth/Média.
     * @param parent Widget ou objet parent pour la gestion de la mémoire.
     */
    explicit BluetoothManager(QObject *parent = nullptr);

    // --- GETTERS ---
    QString title() const { return m_title; }       ///< Retourne le titre de la piste actuelle.
    QString artist() const { return m_artist; }     ///< Retourne le nom de l'artiste.
    QString album() const { return m_album; }       ///< Retourne le nom de l'album.
    bool isPlaying() const { return m_isPlaying; }  ///< Indique si la musique est en cours de lecture.
    qint64 positionMs() const { return m_positionMs; } ///< Retourne la position actuelle dans la piste (en millisecondes).
    qint64 durationMs() const { return m_durationMs; } ///< Retourne la durée totale de la piste (en millisecondes).

public slots:
    // --- COMMANDES MÉDIA ---
    void next();        ///< Passe à la piste suivante.
    void previous();    ///< Revient à la piste précédente.
    void togglePlay();  ///< Bascule entre Lecture et Pause.

signals:
    // --- SIGNAUX DE NOTIFICATION ---
    void metadataChanged(); ///< Émis lorsque la chanson, l'artiste ou l'album change.
    void statusChanged();   ///< Émis lorsque l'état de lecture change (Play -> Pause).
    void positionChanged(); ///< Émis lorsque la position de lecture avance.

private slots:
    /**
     * @brief Slot déclenché lors de la réception d'un signal DBus `PropertiesChanged`.
     * @param msg Le message DBus brut contenant les propriétés modifiées.
     */
    void handleDBusSignal(const QDBusMessage &msg);

private:
    // --- MÉTHODES INTERNES ---

    void updateMetadata();       ///< Interroge explicitement le lecteur pour obtenir toutes les métadonnées.
    void updatePlaybackStatus(); ///< Interroge explicitement le lecteur pour connaître son état de lecture.
    void updatePosition();       ///< Interroge explicitement le lecteur pour obtenir la position de lecture exacte.

    void findActivePlayer();     ///< Scanne le bus pour trouver un lecteur MPRIS actif auquel se connecter.

    /**
     * @brief Établit la connexion DBus avec un lecteur média spécifique.
     * @param service Le nom du service DBus (ex: "org.mpris.MediaPlayer2.spotify").
     */
    void connectToService(const QString &service);

    /**
     * @brief Analyse un dictionnaire de métadonnées MPRIS et met à jour les variables internes.
     * @param metadata Dictionnaire (Map) contenant les tags audio (xesam:title, xesam:artist, etc.).
     */
    void parseMetadataMap(const QVariantMap &metadata);

    // --- ATTRIBUTS ---
    QString m_currentService;        ///< Nom du service DBus actuellement suivi.
    QString m_title = "En attente..."; ///< Titre de la piste en cours.
    QString m_artist = "";           ///< Artiste de la piste en cours.
    QString m_album = "";            ///< Album de la piste en cours.
    bool m_isPlaying = false;        ///< État de la lecture.

    qint64 m_positionMs = 0;         ///< Position de lecture (en ms).
    qint64 m_durationMs = 0;         ///< Durée totale (en ms).

    QDBusInterface *m_playerInterface = nullptr; ///< Interface de communication active avec le lecteur.
};

#endif
