#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <QTimer>

// Fonction de déballage pour lire les données complexes du téléphone
QVariant unwrapVariant(const QVariant &var) {
    // QDBusVariant -> QVariant
    if (var.userType() == qMetaTypeId<QDBusVariant>()) {
        return unwrapVariant(var.value<QDBusVariant>().variant());
    }

    // QDBusArgument -> QVariant (variant, map, array…)
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
            // unwrap récursif des valeurs
            for (auto it = map.begin(); it != map.end(); ++it)
                it.value() = unwrapVariant(it.value());
            return map;
        }

        if (arg.currentType() == QDBusArgument::ArrayType) {
            // Lire en QVariantList (plus général que QStringList)
            QVariantList list;
            arg >> list;
            for (QVariant &v : list)
                v = unwrapVariant(v);
            return list;
        }
    }

    // Si c’est déjà une QVariantList, unwrap récursif
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

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(
        "org.mpris.MediaPlayer2*",
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this
        );

    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &BluetoothManager::connectToService);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service){
        if (service == m_currentService) {
            m_title = "Déconnecté";
            m_artist = "";
            m_isPlaying = false;
            m_currentService.clear();
            emit metadataChanged();
            emit statusChanged();

            // 🔁 Re-scan et reconnexion auto
            QTimer::singleShot(300, this, &BluetoothManager::findActivePlayer);
        }
    });


    findActivePlayer();
}

void BluetoothManager::findActivePlayer() {
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus) return;
    QStringList services = bus->registeredServiceNames();
    for (const QString &service : services) {
        if (service.startsWith("org.mpris.MediaPlayer2.") &&
            !service.contains("mpris-proxy") &&
            !service.contains("Bluetooth_Player")) {
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

    // Connexion avec QDBusMessage pour ne rater aucun signal de changement
    QDBusConnection::sessionBus().connect(serviceName, "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                          this, SLOT(handleDBusSignal(QDBusMessage)));

    updateMetadata();
}

void BluetoothManager::handleDBusSignal(const QDBusMessage &msg) {
    QList<QVariant> args = msg.arguments();
    if (args.count() < 2) return;

    QString interface = args.at(0).toString();
    if (interface != "org.mpris.MediaPlayer2.Player") return;

    QVariantMap changedProperties = unwrapVariant(args.at(1)).toMap();

    if (changedProperties.contains("Metadata")) {
        qDebug() << "📢 SIGNAL REÇU : Changement de musique détecté !";
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
    QString newTitle = unwrapVariant(metadata.value("xesam:title")).toString();

    QString newArtist;
    QVariant artistVar = unwrapVariant(metadata.value("xesam:artist"));

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

    if (!newTitle.isEmpty()) {
        m_title = newTitle;
        m_artist = newArtist;
        qDebug() << "🎵 Musique actuelle :" << m_title << "par" << m_artist;
        emit metadataChanged();
    }
}

void BluetoothManager::togglePlay() { if(m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { if(m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous() { if(m_playerInterface) m_playerInterface->call("Previous"); }
