#include "bluetoothmanager.h"
#include <QDebug>

BluetoothManager::BluetoothManager(QObject *parent) : QObject(parent) {
    // CORRECTION : Le service s'appelle "...bluez" quand on utilise mpris-proxy
    QString serviceName = "org.mpris.MediaPlayer2.bluez";

    // 1. Interface pour les commandes (Next/Pause)
    m_playerInterface = new QDBusInterface(serviceName,
                                           "/org/mpris/MediaPlayer2",
                                           "org.mpris.MediaPlayer2.Player",
                                           QDBusConnection::sessionBus(), this);

    // 2. Connexion pour �couter les m�tadonn�es (Titre/Artiste)
    QDBusConnection::sessionBus().connect(serviceName,
                                          "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          this, SLOT(handleDBusSignal(QString, QVariantMap, QStringList)));

    // Petit log pour confirmer au d�marrage
    if (m_playerInterface->isValid()) {
        qDebug() << "? BluetoothManager: Connect� �" << serviceName;
    } else {
        qWarning() << "?? BluetoothManager: Impossible de joindre" << serviceName
                   << "(V�rifie que la musique est lanc�e sur le t�l�phone)";
    }

    updateMetadata();
}

void BluetoothManager::handleDBusSignal(const QString &interface, const QVariantMap &changedProperties, const QStringList &) {
    if (interface != "org.mpris.MediaPlayer2.Player") return;

    // R�cup�ration du Titre / Artiste
    if (changedProperties.contains("Metadata")) {
        QVariantMap metadata = qvariant_cast<QDBusVariant>(changedProperties["Metadata"]).variant().toMap();
        m_title = metadata["xesam:title"].toString();
        m_artist = metadata["xesam:artist"].toStringList().join(", ");

        qDebug() << "?? Nouvelle musique :" << m_title << "-" << m_artist;
        emit metadataChanged();
    }

    // R�cup�ration du statut Lecture/Pause
    if (changedProperties.contains("PlaybackStatus")) {
        m_isPlaying = (changedProperties["PlaybackStatus"].toString() == "Playing");
        emit statusChanged();
    }
}

void BluetoothManager::togglePlay() { m_playerInterface->call("PlayPause"); }
void BluetoothManager::next() { m_playerInterface->call("Next"); }
void BluetoothManager::previous() { m_playerInterface->call("Previous"); }

void BluetoothManager::updateMetadata() {
    if (m_playerInterface->isValid()) {
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
}
