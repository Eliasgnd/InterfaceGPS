/**
 * @file bluetoothmanager.cpp
 * @brief Implémentation DBus du gestionnaire multimédia Bluetooth.
 * @details Responsabilités : Découvrir un lecteur MPRIS actif, synchroniser les métadonnées
 * et piloter la lecture. Utilise des mécanismes avancés de décodage des QVariant imbriqués de DBus.
 * Dépendances principales : QDBusConnection, QDBusInterface, QDBusServiceWatcher et le modèle MPRIS.
 */

#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <QTimer>

/**
 * @brief Normalise les enveloppes DBus imbriquées pour simplifier le parsing MPRIS.
 * @details DBus a tendance à encapsuler les types complexes dans de multiples couches de QVariant.
 * Cette fonction récursive "déshabille" ces variables pour obtenir le type C++ primitif utilisable.
 * @param var Le QVariant potentiellement encapsulé reçu de DBus.
 * @return Un QVariant purifié et exploitable directement.
 */
static QVariant unwrapVariant(const QVariant &var) {
    if (var.userType() == qMetaTypeId<QDBusVariant>()) {
        return unwrapVariant(var.value<QDBusVariant>().variant());
    }

    if (var.userType() == qMetaTypeId<QDBusArgument>()) {
        QDBusArgument arg = var.value<QDBusArgument>();

        if (arg.currentType() == QDBusArgument::VariantType) {
            QVariant inner;
            arg >> inner;
            return unwrapVariant(inner);
        }

        if (arg.currentType() == QDBusArgument::MapType) {
            QVariantMap map;
            arg >> map;
            for (auto it = map.begin(); it != map.end(); ++it)
                it.value() = unwrapVariant(it.value());
            return map;
        }

        if (arg.currentType() == QDBusArgument::ArrayType) {
            QVariantList list;
            arg >> list;
            for (QVariant &v : list)
                v = unwrapVariant(v);
            return list;
        }
    }

    if (var.typeId() == QMetaType::QVariantList) {
        QVariantList list = var.toList();
        for (QVariant &v : list)
            v = unwrapVariant(v);
        return list;
    }

    return var;
}

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    // Enregistrement des types complexes requis par le système QtDBus
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<QList<QVariant>>();

    // Création d'un Watcher pour être alerté dès qu'un lecteur multimédia s'allume ou s'éteint
    auto *watcher = new QDBusServiceWatcher(
        "org.mpris.MediaPlayer2*",
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this
        );

    connect(watcher, &QDBusServiceWatcher::serviceRegistered,
            this, &BluetoothManager::connectToService);

    connect(watcher, &QDBusServiceWatcher::serviceUnregistered,
            this, [this](const QString &service){
                if (service == m_currentService) {
                    // Si le lecteur actuel se déconnecte, on réinitialise l'interface
                    m_title = "Déconnecté";
                    m_artist = "";
                    m_album = "";
                    m_isPlaying = false;
                    m_positionMs = 0;
                    m_durationMs = 0;

                    m_currentService.clear();
                    emit metadataChanged();
                    emit statusChanged();
                    emit positionChanged();

                    // On patiente un instant puis on cherche si un autre lecteur a pris le relais
                    QTimer::singleShot(300, this, &BluetoothManager::findActivePlayer);
                }
            });

    findActivePlayer();
}

void BluetoothManager::findActivePlayer() {
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus) return;

    const QStringList services = bus->registeredServiceNames();

    // Priorité 1 : On cherche un "vrai" lecteur multimédia actif.
    // Certains wrappers MPRIS (comme mpris-proxy) exposent des services éphémères ;
    // ce filtre réduit les bascules de lecteur intempestives qui dégradent l'UX sur des reconnexions rapides.
    for (const QString &service : services) {
        if (service.startsWith("org.mpris.MediaPlayer2.") &&
            !service.contains("mpris-proxy") &&
            !service.contains("Bluetooth_Player")) {
            connectToService(service);
            return;
        }
    }

    // Priorité 2 (Repli) : Si aucun vrai lecteur n'est trouvé, on accepte les proxy Bluetooth.
    // On préfère un lecteur potentiellement imparfait plutôt qu'une UI vide.
    for (const QString &service : services) {
        if (service.startsWith("org.mpris.MediaPlayer2.") &&
            !service.endsWith(".mpris-proxy")) {
            connectToService(service);
            return;
        }
    }
}

void BluetoothManager::connectToService(const QString &serviceName) {
    if (serviceName.isEmpty()) return;
    if (serviceName == m_currentService && m_playerInterface) return;

    qDebug() << " Connexion au lecteur :" << serviceName;
    m_currentService = serviceName;

    // Une seule subscription PropertiesChanged est conservée pour éviter les doublons de notifications
    // (on nettoie l'ancienne écoute avant d'en créer une nouvelle).
    QDBusConnection::sessionBus().disconnect(QString(), "/org/mpris/MediaPlayer2",
                                             "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                             this, SLOT(handleDBusSignal(QDBusMessage)));

    delete m_playerInterface;

    // Création de l'interface qui nous permet d'envoyer des commandes (Play/Pause)
    m_playerInterface = new QDBusInterface(
        serviceName,
        "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player",
        QDBusConnection::sessionBus(),
        this
        );

    // Connexion pour écouter les changements asynchrones (ex: l'utilisateur a changé de musique sur son téléphone)
    QDBusConnection::sessionBus().connect(
        serviceName,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        this,
        SLOT(handleDBusSignal(QDBusMessage))
        );

    // Initialisation immédiate de l'état (récupération des données courantes)
    updateMetadata();
    updatePlaybackStatus();
    updatePosition();
}

void BluetoothManager::handleDBusSignal(const QDBusMessage &msg) {
    // Un seul point d'entrée capte Metadata/PlaybackStatus/Position pour garder une synchronisation atomique.
    const QList<QVariant> args = msg.arguments();
    if (args.size() < 2) return;

    const QString iface = args.at(0).toString();
    if (iface != "org.mpris.MediaPlayer2.Player") return;

    const QVariantMap changed = unwrapVariant(args.at(1)).toMap();

    if (changed.contains("Metadata")) {
        parseMetadataMap(unwrapVariant(changed.value("Metadata")).toMap());
    }

    if (changed.contains("PlaybackStatus")) {
        m_isPlaying = (unwrapVariant(changed.value("PlaybackStatus")).toString() == "Playing");
        emit statusChanged();

        if (m_isPlaying) {
            updatePosition();
        }
    }

    if (changed.contains("Position")) {
        const qint64 posUs = unwrapVariant(changed.value("Position")).toLongLong();
        m_positionMs = posUs / 1000;
        emit positionChanged();
    }
}

void BluetoothManager::updateMetadata() {
    if (m_currentService.isEmpty()) return;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_currentService,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "Get"
        );
    msg << "org.mpris.MediaPlayer2.Player" << "Metadata";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        parseMetadataMap(unwrapVariant(reply.arguments().first()).toMap());
    }
}

void BluetoothManager::updatePlaybackStatus() {
    if (m_currentService.isEmpty()) return;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_currentService,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "Get"
        );
    msg << "org.mpris.MediaPlayer2.Player" << "PlaybackStatus";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        m_isPlaying = (unwrapVariant(reply.arguments().first()).toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::updatePosition() {
    if (m_currentService.isEmpty()) return;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_currentService,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "Get"
        );
    msg << "org.mpris.MediaPlayer2.Player" << "Position";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        // La position est fournie en microsecondes par MPRIS,
        // on la convertit en millisecondes pour notre binding QML.
        const qint64 posUs = unwrapVariant(reply.arguments().first()).toLongLong();
        m_positionMs = posUs / 1000;
        emit positionChanged();
    }
}

void BluetoothManager::parseMetadataMap(const QVariantMap &metadata) {
    const QString newTitle = unwrapVariant(metadata.value("xesam:title")).toString();
    const QString newAlbum = unwrapVariant(metadata.value("xesam:album")).toString();

    QString newArtist;
    const QVariant artistVar = unwrapVariant(metadata.value("xesam:artist"));

    // L'artiste peut être un simple String ou une Liste de Strings (feat.)
    if (artistVar.canConvert<QStringList>()) {
        newArtist = artistVar.toStringList().join(", ");
    } else if (artistVar.typeId() == QMetaType::QVariantList) {
        QStringList tmp;
        for (const QVariant &it : artistVar.toList())
            tmp << it.toString();
        newArtist = tmp.join(", ");
    } else {
        newArtist = artistVar.toString();
    }

    const qint64 lenUs = unwrapVariant(metadata.value("mpris:length")).toLongLong();
    const qint64 newDurationMs = (lenUs > 0) ? (lenUs / 1000) : 0;

    bool changed = false;

    // On vérifie s'il y a eu un vrai changement avant d'émettre les signaux
    if (!newTitle.isEmpty() && newTitle != m_title) { m_title = newTitle; changed = true; }
    if (newArtist != m_artist) { m_artist = newArtist; changed = true; }
    if (newAlbum != m_album) { m_album = newAlbum; changed = true; }
    if (newDurationMs != m_durationMs) { m_durationMs = newDurationMs; changed = true; }

    if (changed) {
        qDebug() << "🎵" << m_title << "-" << m_artist << "(" << m_album << ")";
        emit metadataChanged();
    }
}

void BluetoothManager::togglePlay() { if (m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next()       { if (m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous()   { if (m_playerInterface) m_playerInterface->call("Previous"); }
