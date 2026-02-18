#include "bluetoothmanager.h"
#include <QDebug>

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    // Connexion à l'interface MPRIS générée par mpris-proxy
    m_playerInterface = new QDBusInterface("org.mpris.MediaPlayer2.mpris-proxy",
                                           "/org/mpris/MediaPlayer2",
                                           "org.mpris.MediaPlayer2.Player",
                                           QDBusConnection::sessionBus(), this);

    // Écouter les changements de propriétés (nouveau titre, pause, etc.)
    QDBusConnection::sessionBus().connect("org.mpris.MediaPlayer2.mpris-proxy",
                                          "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          this, SLOT(handleDBusSignal(QString, QVariantMap, QStringList)));

    updateMetadata();
}

void BluetoothManager::handleDBusSignal(const QString &interface, const QVariantMap &changedProperties, const QStringList &) {
    if (interface != "org.mpris.MediaPlayer2.Player") return;

    if (changedProperties.contains("Metadata")) {
        QVariantMap metadata = qvariant_cast<QDBusVariant>(changedProperties["Metadata"]).variant().toMap();
        m_title = metadata["xesam:title"].toString();
        m_artist = metadata["xesam:artist"].toStringList().join(", ");
        emit metadataChanged();
    }

    if (changedProperties.contains("PlaybackStatus")) {
        m_isPlaying = (changedProperties["PlaybackStatus"].toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::togglePlay() { m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { m_playerInterface->call("Next"); }
void BluetoothManager::previous() { m_playerInterface->call("Previous"); }

void BluetoothManager::updateMetadata() {
    // Initialisation au démarrage
    if (m_playerInterface->isValid()) {
        QVariant res = m_playerInterface->property("Metadata");
        // ... (extraction similaire à handleDBusSignal)
    }
}
