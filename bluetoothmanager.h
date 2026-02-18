#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <QObject>
#include <QtDBus/QtDBus>

class BluetoothManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ title NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metadataChanged)
    Q_PROPERTY(QString album READ album NOTIFY metadataChanged)

    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY statusChanged)

    // Temps (ms) pour le QML
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY metadataChanged)

public:
    explicit BluetoothManager(QObject *parent = nullptr);

    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }

    bool isPlaying() const { return m_isPlaying; }

    qint64 positionMs() const { return m_positionMs; }
    qint64 durationMs() const { return m_durationMs; }

public slots:
    void next();
    void previous();
    void togglePlay();

signals:
    void metadataChanged();
    void statusChanged();
    void positionChanged();

private slots:
    void handleDBusSignal(const QDBusMessage &msg);

private:
    void updateMetadata();
    void updatePlaybackStatus();
    void updatePosition();

    void findActivePlayer();
    void connectToService(const QString &service);

    void parseMetadataMap(const QVariantMap &metadata);

    QString m_currentService;
    QString m_title = "En attente...";
    QString m_artist = "";
    QString m_album = "";
    bool m_isPlaying = false;

    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;

    QDBusInterface *m_playerInterface = nullptr;
};

#endif
