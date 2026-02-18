#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusArgument>

// --- FONCTION UTILITAIRE CORRIGÉE ---
QVariant unwrapVariant(const QVariant &var) {
    // CORRECTION : On utilise qMetaTypeId pour identifier le type DBusArgument
    if (var.userType() == qMetaTypeId<QDBusArgument>()) {
        const QDBusArgument &arg = var.value<QDBusArgument>();

        // Si c'est une variante emballée (boîte dans une boîte)
        if (arg.currentType() == QDBusArgument::VariantType) {
            QVariant inner;
            arg >> inner;
            return unwrapVariant(inner);
        }
        // Si c'est un tableau (ex: liste d'artistes)
        if (arg.currentType() == QDBusArgument::ArrayType) {
            QStringList list;
            arg >> list;
            return list;
        }
    }
    return var; // C'est déjà une donnée simple
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
            qDebug() << "⚠️ BluetoothManager: Déconnecté de" << service;
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
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
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
        QVariant rawMeta = changedProperties["Metadata"];
        QVariant unwrapped = unwrapVariant(rawMeta);

        // Conversion sécurisée en Map
        if (unwrapped.canConvert<QVariantMap>()) {
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

    // Appel direct via DBus
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_currentService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
    msg << "org.mpris.MediaPlayer2.Player" << "Metadata";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant rawReply = reply.arguments().first();
        QVariant unwrapped = unwrapVariant(rawReply);
        if (unwrapped.canConvert<QVariantMap>()) {
            parseMetadataMap(unwrapped.toMap());
        }
    }

    // Mise à jour du statut Play/Pause
    QDBusMessage msgStatus = QDBusMessage::createMethodCall(
        m_currentService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
    msgStatus << "org.mpris.MediaPlayer2.Player" << "PlaybackStatus";
    QDBusMessage replyStatus = QDBusConnection::sessionBus().call(msgStatus);
    if (replyStatus.type() == QDBusMessage::ReplyMessage && !replyStatus.arguments().isEmpty()) {
        m_isPlaying = (unwrapVariant(replyStatus.arguments().first()).toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::parseMetadataMap(const QVariantMap &metadata) {
    // Debug des clés reçues
    qDebug() << "--- Données Reçues ---";
    for(auto it = metadata.begin(); it != metadata.end(); ++it) {
        // Conversion de la valeur en string pour affichage debug
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
        QVariant artistVar = unwrapVariant(metadata["xesam:artist"]);
        if (artistVar.canConvert<QStringList>()) {
            newArtist = artistVar.toStringList().join(", ");
        } else {
            newArtist = artistVar.toString();
        }
    } else if (metadata.contains("Artist")) {
        newArtist = unwrapVariant(metadata["Artist"]).toString();
    }

    if (!newTitle.isEmpty()) {
        m_title = newTitle;
        m_artist = newArtist;
        qDebug() << "🎵 SUCCÈS :" << m_title << "-" << m_artist;
        emit metadataChanged();
    }
}

void BluetoothManager::togglePlay() { if(m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { if(m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous() { if(m_playerInterface) m_playerInterface->call("Previous"); }
