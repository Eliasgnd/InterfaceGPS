#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusArgument>

// --- FONCTION DE DÉBALLAGE CORRIGÉE ---
QVariant unwrapVariant(const QVariant &var) {
    // CORRECTION ICI : On utilise qMetaTypeId<T>() au lieu de QMetaType::T
    if (var.userType() == qMetaTypeId<QDBusArgument>()) {
        const QDBusArgument &arg = var.value<QDBusArgument>();

        // Si c'est une "boîte" (Variant DBus)
        if (arg.currentType() == QDBusArgument::VariantType) {
            QVariant inner;
            arg >> inner;
            return unwrapVariant(inner); // On continue de creuser
        }

        // Si c'est une liste (ex: Artistes)
        if (arg.currentType() == QDBusArgument::ArrayType) {
            QStringList list;
            arg >> list;
            return list;
        }

        // Si c'est une Map (ex: Metadata complète)
        if (arg.currentType() == QDBusArgument::MapType) {
            QVariantMap map;
            arg >> map;
            return map;
        }
    }

    // Si c'est un QDBusVariant (autre type d'emballage)
    if (var.userType() == qMetaTypeId<QDBusVariant>()) {
        return unwrapVariant(var.value<QDBusVariant>().variant());
    }

    return var; // C'est une donnée simple (String, Int...)
}

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    // Enregistrement des types
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<QList<QVariant>>();

    // Surveillance des services
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(
        "org.mpris.MediaPlayer2*",
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this
        );

    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &BluetoothManager::connectToService);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service){
        if (service == m_currentService) {
            qDebug() << "⚠️ Déconnecté de" << service;
            m_title = "Déconnecté";
            m_artist = "";
            m_isPlaying = false;
            m_currentService.clear();
            if(m_playerInterface) { delete m_playerInterface; m_playerInterface = nullptr; }
            emit metadataChanged();
            emit statusChanged();
        }
    });

    findActivePlayer();
}

void BluetoothManager::findActivePlayer() {
    QDBusConnectionBusInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus) return;

    QStringList services = bus->registeredServiceNames();
    for (const QString &service : services) {
        if (service.startsWith("org.mpris.MediaPlayer2.") && !service.endsWith(".mpris-proxy")) {
            connectToService(service);
            return;
        }
    }
    qWarning() << "❌ Aucun lecteur trouvé au démarrage.";
}

void BluetoothManager::connectToService(const QString &serviceName) {
    qDebug() << "✅ Nouveau lecteur détecté :" << serviceName;
    m_currentService = serviceName;

    if (m_playerInterface) delete m_playerInterface;
    m_playerInterface = new QDBusInterface(serviceName,
                                           "/org/mpris/MediaPlayer2",
                                           "org.mpris.MediaPlayer2.Player",
                                           QDBusConnection::sessionBus(), this);

    QDBusConnection::sessionBus().connect(serviceName,
                                          "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          this, SLOT(handleDBusSignal(QString, QVariantMap, QStringList)));

    updateMetadata();
}

void BluetoothManager::handleDBusSignal(const QString &interface, const QVariantMap &changedProperties, const QStringList &) {
    if (interface != "org.mpris.MediaPlayer2.Player") return;

    if (changedProperties.contains("Metadata")) {
        // Extraction forcée
        QVariant raw = changedProperties["Metadata"];
        QVariant unwrapped = unwrapVariant(raw);

        if (unwrapped.typeId() == QMetaType::QVariantMap) {
            parseMetadataMap(unwrapped.toMap());
        }
    }

    if (changedProperties.contains("PlaybackStatus")) {
        m_isPlaying = (changedProperties["PlaybackStatus"].toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::updateMetadata() {
    if (m_currentService.isEmpty()) return;

    // Appel direct DBus pour éviter les conversions ratées de Qt
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_currentService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
    msg << "org.mpris.MediaPlayer2.Player" << "Metadata";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant rawReply = reply.arguments().first();
        QVariant unwrapped = unwrapVariant(rawReply);

        if (unwrapped.typeId() == QMetaType::QVariantMap) {
            parseMetadataMap(unwrapped.toMap());
        }
    }

    // Status
    if (m_playerInterface) {
        QVariant s = m_playerInterface->property("PlaybackStatus");
        m_isPlaying = (s.toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::parseMetadataMap(const QVariantMap &metadata) {
    // Debug pour voir ce qu'on reçoit
    qDebug() << "--- Metadonnées Reçues ---";
    for(auto it = metadata.begin(); it != metadata.end(); ++it) {
        qDebug() << "Clé:" << it.key() << "Valeur:" << unwrapVariant(it.value()).toString();
    }

    QString newTitle;
    if (metadata.contains("xesam:title")) {
        newTitle = unwrapVariant(metadata["xesam:title"]).toString();
    } else if (metadata.contains("Title")) {
        newTitle = unwrapVariant(metadata["Title"]).toString();
    }

    QString newArtist;
    if (metadata.contains("xesam:artist")) {
        QVariant v = unwrapVariant(metadata["xesam:artist"]);
        if (v.typeId() == QMetaType::QStringList) newArtist = v.toStringList().join(", ");
        else newArtist = v.toString();
    } else if (metadata.contains("Artist")) {
        newArtist = unwrapVariant(metadata["Artist"]).toString();
    }

    if (!newTitle.isEmpty()) {
        m_title = newTitle;
        m_artist = newArtist;
        emit metadataChanged();
    }
}

void BluetoothManager::togglePlay() { if(m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { if(m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous() { if(m_playerInterface) m_playerInterface->call("Previous"); }
