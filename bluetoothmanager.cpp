#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusArgument>

// --- FONCTION DE DÉBALLAGE ---
QVariant unwrapVariant(const QVariant &var) {
    if (var.userType() == qMetaTypeId<QDBusArgument>()) {
        const QDBusArgument &arg = var.value<QDBusArgument>();
        if (arg.currentType() == QDBusArgument::VariantType) {
            QVariant inner; arg >> inner;
            return unwrapVariant(inner);
        }
        if (arg.currentType() == QDBusArgument::ArrayType) {
            QStringList list; arg >> list;
            return list;
        }
        if (arg.currentType() == QDBusArgument::MapType) {
            QVariantMap map; arg >> map;
            return map;
        }
    }
    if (var.userType() == qMetaTypeId<QDBusVariant>()) {
        return unwrapVariant(var.value<QDBusVariant>().variant());
    }
    return var;
}

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<QList<QVariant>>();

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(
        "org.mpris.MediaPlayer2*",
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this
        );

    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &BluetoothManager::connectToService);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service){
        if (service == m_currentService) {
            qDebug() << "⚠️ Appareil déconnecté";
            m_title = "Déconnecté"; m_artist = ""; m_isPlaying = false;
            m_currentService.clear();
            emit metadataChanged(); emit statusChanged();
        }
    });

    findActivePlayer();
}

void BluetoothManager::findActivePlayer() {
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus) return;
    QStringList services = bus->registeredServiceNames();
    for (const QString &service : services) {
        if (service.startsWith("org.mpris.MediaPlayer2.") && !service.endsWith(".mpris-proxy")) {
            connectToService(service);
            return;
        }
    }
}

void BluetoothManager::connectToService(const QString &serviceName) {
    qDebug() << "✅ Connexion au lecteur :" << serviceName;
    m_currentService = serviceName;

    if (m_playerInterface) delete m_playerInterface;
    m_playerInterface = new QDBusInterface(serviceName, "/org/mpris/MediaPlayer2",
                                           "org.mpris.MediaPlayer2.Player", QDBusConnection::sessionBus(), this);

    // CONNEXION SIGNAL : On utilise la signature générique QDBusMessage pour ne rien rater
    QDBusConnection::sessionBus().connect(serviceName, "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                          this, SLOT(handleDBusSignal(QDBusMessage)));

    updateMetadata();
}

// NOUVEAU SLOT DE RÉCEPTION UNIVERSEL
void BluetoothManager::handleDBusSignal(const QDBusMessage &msg) {
    QList<QVariant> args = msg.arguments();
    if (args.count() < 2) return;

    QString interface = args.at(0).toString();
    if (interface != "org.mpris.MediaPlayer2.Player") return;

    QVariantMap changedProperties = unwrapVariant(args.at(1)).toMap();

    if (changedProperties.contains("Metadata")) {
        qDebug() << "📢 Signal de changement de musique reçu !";
        parseMetadataMap(unwrapVariant(changedProperties["Metadata"]).toMap());
    }

    if (changedProperties.contains("PlaybackStatus")) {
        m_isPlaying = (unwrapVariant(changedProperties["PlaybackStatus"]).toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::updateMetadata() {
    if (m_currentService.isEmpty()) return;

    QDBusMessage msg = QDBusMessage::createMethodCall(m_currentService, "/org/mpris/MediaPlayer2",
                                                      "org.freedesktop.DBus.Properties", "Get");
    msg << "org.mpris.MediaPlayer2.Player" << "Metadata";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        parseMetadataMap(unwrapVariant(reply.arguments().first()).toMap());
    }
}

void BluetoothManager::parseMetadataMap(const QVariantMap &metadata) {
    QString newTitle = unwrapVariant(metadata["xesam:title"]).toString();
    QString newArtist;
    QVariant v = unwrapVariant(metadata["xesam:artist"]);
    if (v.userType() == QMetaType::QStringList) newArtist = v.toStringList().join(", ");
    else newArtist = v.toString();

    // On vérifie si c'est vraiment différent avant de mettre à jour
    if (!newTitle.isEmpty() && (newTitle != m_title || newArtist != m_artist)) {
        m_title = newTitle;
        m_artist = newArtist;
        qDebug() << "🎵 Nouvelle Musique :" << m_title << "par" << m_artist;
        emit metadataChanged(); // C'est ce signal qui dit au QML de se rafraîchir
    }
}

void BluetoothManager::togglePlay() { if(m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { if(m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous() { if(m_playerInterface) m_playerInterface->call("Previous"); }
