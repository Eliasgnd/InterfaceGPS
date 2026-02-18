#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusArgument>

// Fonction utilitaire pour "déballer" les données complexes de Linux (DBus)
QVariant unwrapVariant(const QVariant &var) {
    if (var.userType() == QMetaType::QDBusArgument) {
        const QDBusArgument &arg = var.value<QDBusArgument>();
        if (arg.currentType() == QDBusArgument::VariantType) {
            QVariant inner;
            arg >> inner;
            return unwrapVariant(inner); // On continue de creuser si nécessaire
        }
        if (arg.currentType() == QDBusArgument::ArrayType) {
            QStringList list;
            arg >> list; // On essaie de lire une liste de chaînes (ex: Artistes)
            return list;
        }
    }
    return var; // C'est déjà une donnée simple (String, Int, etc.)
}

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    // Enregistrement des types complexes pour éviter les erreurs
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
            qDebug() << "⚠️ Déconnecté de :" << service;
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
        // On récupère la map brute et on l'analyse
        QVariant rawMeta = changedProperties["Metadata"];
        QVariant unwrapped = unwrapVariant(rawMeta);

        if (unwrapped.typeId() == QMetaType::QVariantMap) {
            parseMetadataMap(unwrapped.toMap());
        } else {
            // Fallback : parfois la map est directement accessible
            parseMetadataMap(rawMeta.toMap());
        }
    }

    if (changedProperties.contains("PlaybackStatus")) {
        m_isPlaying = (changedProperties["PlaybackStatus"].toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::updateMetadata() {
    if (m_currentService.isEmpty()) return;

    // Appel manuel direct via DBus pour contourner les bugs de Qt
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_currentService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
    msg << "org.mpris.MediaPlayer2.Player" << "Metadata";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant rawReply = reply.arguments().first();
        // Le premier niveau est un QDBusVariant qui contient la Map
        QVariant unwrapped = unwrapVariant(rawReply);
        parseMetadataMap(unwrapped.toMap());
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
    // DEBUG : Affiche toutes les clés reçues pour comprendre ce qui se passe
    qDebug() << "--- Réception Métadonnées ---";
    for(auto it = metadata.begin(); it != metadata.end(); ++it) {
        qDebug() << "Clé:" << it.key() << "Valeur (Type):" << it.value().typeName() << it.value();
    }

    // 1. Récupération du Titre (avec déballage forcé)
    QString newTitle;
    if (metadata.contains("xesam:title")) {
        newTitle = unwrapVariant(metadata["xesam:title"]).toString();
    } else if (metadata.contains("Title")) { // Cas rare
        newTitle = unwrapVariant(metadata["Title"]).toString();
    }

    // 2. Récupération de l'Artiste
    QString newArtist;
    if (metadata.contains("xesam:artist")) {
        QVariant artistVar = unwrapVariant(metadata["xesam:artist"]);
        if (artistVar.typeId() == QMetaType::QStringList) {
            newArtist = artistVar.toStringList().join(", ");
        } else {
            newArtist = artistVar.toString();
        }
    } else if (metadata.contains("Artist")) {
        newArtist = unwrapVariant(metadata["Artist"]).toString();
    }

    // Mise à jour si non vide
    if (!newTitle.isEmpty()) {
        m_title = newTitle;
        m_artist = newArtist;
        qDebug() << "🎵 SUCCÈS :" << m_title << "-" << m_artist;
        emit metadataChanged();
    } else {
        qWarning() << "⚠️ Titre vide ou format inconnu.";
    }
}

void BluetoothManager::togglePlay() { if(m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { if(m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous() { if(m_playerInterface) m_playerInterface->call("Previous"); }
