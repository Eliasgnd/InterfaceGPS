#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <QObject>
#include <QtDBus/QtDBus>

class BluetoothManager : public QObject {
    Q_OBJECT
    // Propriétés lisibles par le QML
    Q_PROPERTY(QString title READ title NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metadataChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY statusChanged)

public:
    explicit BluetoothManager(QObject *parent = nullptr);

    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    bool isPlaying() const { return m_isPlaying; }

public slots:
    // Commandes envoyées au téléphone
    void next();
    void previous();
    void togglePlay();

signals:
    void metadataChanged();
    void statusChanged();

private slots:
    void handleDBusSignal(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void updateMetadata();

    QString m_title = "Pas de musique";
    QString m_artist = "Déconnecté";
    bool m_isPlaying = false;
    QDBusInterface *m_playerInterface = nullptr;
};

#endif
