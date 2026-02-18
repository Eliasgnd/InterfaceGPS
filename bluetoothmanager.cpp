#include "bluetoothmanager.h"
#include <QDebug>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    // 1. On surveille TOUT ce qui ressemble à un lecteur média
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(
        "org.mpris.MediaPlayer2*",
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this
        );

    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &BluetoothManager::connectToService);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service){
        if (service == m_currentService) {
            qDebug() << "⚠️ BluetoothManager: Appareil déconnecté (" << service << ")";
            m_title = "Déconnecté";
            m_artist = "";
            m_isPlaying = false;
            m_currentService.clear();
            if(m_playerInterface) { delete m_playerInterface; m_playerInterface = nullptr; }
            emit metadataChanged();
            emit statusChanged();
        }
    });

    // 2. Scan au démarrage
    findActivePlayer();
}

void BluetoothManager::findActivePlayer() {
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    for (const QString &service : services) {
        // On prend le premier service qui commence par org.mpris.MediaPlayer2
        if (service.startsWith("org.mpris.MediaPlayer2.") && !service.endsWith(".mpris-proxy")) {
            connectToService(service);
            return;
        }
    }
    qWarning() << "❌ BluetoothManager: Aucun lecteur trouvé pour l'instant.";
}

void BluetoothManager::connectToService(const QString &serviceName) {
    // CORRECTION : On a retiré le filtre "bluez" qui bloquait ton OnePlus
    qDebug() << "✅ BluetoothManager: Connexion à" << serviceName;
    m_currentService = serviceName;

    if (m_playerInterface) delete m_playerInterface;
    m_playerInterface = new QDBusInterface(serviceName,
                                           "/org/mpris/MediaPlayer2",
                                           "org.mpris.MediaPlayer2.Player",
                                           QDBusConnection::sessionBus(), this);

    // Connexion aux signaux
    QDBusConnection::sessionBus().connect(serviceName,
                                          "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          this, SLOT(handleDBusSignal(QString, QVariantMap, QStringList)));

    // Mise à jour immédiate
    updateMetadata();
}

void BluetoothManager::handleDBusSignal(const QString &interface, const QVariantMap &changedProperties, const QStringList &) {
    // On ignore les signaux qui ne concernent pas le lecteur
    if (interface != "org.mpris.MediaPlayer2.Player") return;

    if (changedProperties.contains("Metadata")) {
        QVariantMap metadata = qvariant_cast<QDBusVariant>(changedProperties["Metadata"]).variant().toMap();

        // Parfois le titre est vide au début, on met une sécurité
        QString newTitle = metadata["xesam:title"].toString();
        if (!newTitle.isEmpty()) {
            m_title = newTitle;
            m_artist = metadata["xesam:artist"].toStringList().join(", ");
            qDebug() << "🎵 Titre :" << m_title;
            emit metadataChanged();
        }
    }

    if (changedProperties.contains("PlaybackStatus")) {
        m_isPlaying = (changedProperties["PlaybackStatus"].toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::updateMetadata() {
    if (!m_playerInterface || !m_playerInterface->isValid()) return;

    QVariant metaVar = m_playerInterface->property("Metadata");
    if (metaVar.isValid()) {
        QVariantMap metadata = qvariant_cast<QDBusVariant>(metaVar).variant().toMap();
        m_title = metadata["xesam:title"].toString();
        m_artist = metadata["xesam:artist"].toStringList().join(", ");
        emit metadataChanged();
    }

    QVariant statusVar = m_playerInterface->property("PlaybackStatus");
    if (statusVar.isValid()) {
        m_isPlaying = (statusVar.toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::togglePlay() { if(m_playerInterface) m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { if(m_playerInterface) m_playerInterface->call("Next"); }
void BluetoothManager::previous() { if(m_playerInterface) m_playerInterface->call("Previous"); }
