#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    // 1. Enregistrement vital pour éviter l'erreur "QDBusRawType"
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<QList<QVariant>>();

    // 2. Surveillance des appareils
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(
        "org.mpris.MediaPlayer2*",
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this
        );

    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &BluetoothManager::connectToService);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service){
        if (service == m_currentService) {
            qDebug() << "⚠️ BluetoothManager: Déconnecté (" << service << ")";
            m_title = "En attente...";
            m_artist = "Connectez un téléphone";
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
    qWarning() << "❌ BluetoothManager: Aucun téléphone trouvé pour l'instant.";
}

void BluetoothManager::connectToService(const QString &serviceName) {
    qDebug() << "✅ BluetoothManager: Connexion à" << serviceName;
    m_currentService = serviceName;

    if (m_playerInterface) delete m_playerInterface;
    // On se connecte à l'interface principale du lecteur
    m_playerInterface = new QDBusInterface(serviceName,
                                           "/org/mpris/MediaPlayer2",
                                           "org.mpris.MediaPlayer2.Player",
                                           QDBusConnection::sessionBus(), this);

    // Connexion au signal de changement des propriétés
    QDBusConnection::sessionBus().connect(serviceName,
                                          "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          this, SLOT(handleDBusSignal(QString, QVariantMap, QStringList)));

    // Mise à jour initiale
    updateMetadata();
}

void BluetoothManager::handleDBusSignal(const QString &interface, const QVariantMap &changedProperties, const QStringList &) {
    // Note: L'interface ici peut être "org.mpris.MediaPlayer2.Player"
    if (interface != "org.mpris.MediaPlayer2.Player") return;

    if (changedProperties.contains("Metadata")) {
        // Extraction robuste
        QVariant metaVar = changedProperties["Metadata"];
        QDBusArgument arg = metaVar.value<QDBusArgument>();
        QVariantMap metadata;

        // Si c'est un argument DBus brut, on le convertit
        if (arg.currentType() == QDBusArgument::MapType) {
            arg >> metadata;
        } else {
            metadata = metaVar.toMap(); // Tentative standard
        }

        QString newTitle = metadata["xesam:title"].toString();
        if (!newTitle.isEmpty()) {
            m_title = newTitle;
            m_artist = metadata["xesam:artist"].toStringList().join(", ");
            qDebug() << "🎵 Titre (Signal) :" << m_title;
            emit metadataChanged();
        }
    }

    if (changedProperties.contains("PlaybackStatus")) {
        m_isPlaying = (changedProperties["PlaybackStatus"].toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::updateMetadata() {
    if (m_currentService.isEmpty()) return;

    // MÉTHODE ROBUSTE : Appel direct à "Get" sur l'interface Properties
    // Cela contourne le bug de property() qui échoue sur les types complexes
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_currentService,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "Get"
        );
    msg << "org.mpris.MediaPlayer2.Player" << "Metadata";

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);

    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        // La réponse est un QDBusVariant qui contient notre Map
        QDBusVariant dbusVar = reply.arguments().first().value<QDBusVariant>();
        QVariant val = dbusVar.variant();

        QVariantMap metadata;
        if (val.userType() == qMetaTypeId<QDBusArgument>()) {
            val.value<QDBusArgument>() >> metadata;
        } else {
            metadata = val.toMap();
        }

        m_title = metadata["xesam:title"].toString();
        m_artist = metadata["xesam:artist"].toStringList().join(", ");

        if (m_title.isEmpty()) m_title = "Pause / Inconnu";
        qDebug() << "🎵 Titre (Update) :" << m_title;
        emit metadataChanged();
    } else {
        qWarning() << "⚠️ Échec lecture métadonnées :" << reply.errorMessage();
    }

    // Récupération du statut (Lecture/Pause) - plus simple car c'est une string
    if (m_playerInterface) {
        QVariant status = m_playerInterface->property("PlaybackStatus");
        m_isPlaying = (status.toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::togglePlay() { if(m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { if(m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous() { if(m_playerInterface) m_playerInterface->call("Previous"); }
