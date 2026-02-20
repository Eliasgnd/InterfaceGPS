// Rôle architectural: implémentation DBus du gestionnaire multimédia Bluetooth.
// Responsabilités: découvrir un lecteur MPRIS actif, synchroniser les métadonnées et piloter la lecture.
// Dépendances principales: QDBusConnection, QDBusInterface, QDBusServiceWatcher et modèle MPRIS.

#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <QTimer>

// Normalise les enveloppes DBus imbriquées pour simplifier le parsing MPRIS.
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
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<QList<QVariant>>();

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


                    QTimer::singleShot(300, this, &BluetoothManager::findActivePlayer);
                }
            });

    findActivePlayer();
}

void BluetoothManager::findActivePlayer() {
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus) return;

    const QStringList services = bus->registeredServiceNames();


    // Certains wrappers MPRIS exposent des services éphémères; ce filtre
    // réduit les bascules de lecteur qui dégradent l'UX sur des reconnections rapides.
    for (const QString &service : services) {
        if (service.startsWith("org.mpris.MediaPlayer2.") &&
            !service.contains("mpris-proxy") &&
            !service.contains("Bluetooth_Player")) {
            connectToService(service);
            return;
        }
    }


    // Repli volontaire: on préfère un lecteur potentiellement imparfait plutôt qu'une UI vide.
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

    qDebug() << "✅ Connexion au lecteur :" << serviceName;
    m_currentService = serviceName;


    // Une seule subscription PropertiesChanged est conservée pour éviter les doublons de notifications.
    QDBusConnection::sessionBus().disconnect(QString(), "/org/mpris/MediaPlayer2",
                                             "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                             this, SLOT(handleDBusSignal(QDBusMessage)));

    delete m_playerInterface;
    m_playerInterface = new QDBusInterface(
        serviceName,
        "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player",
        QDBusConnection::sessionBus(),
        this
        );


    QDBusConnection::sessionBus().connect(
        serviceName,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        this,
        SLOT(handleDBusSignal(QDBusMessage))
        );


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
    // Position fournie en microsecondes par MPRIS, convertie en millisecondes pour le binding QML.
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
