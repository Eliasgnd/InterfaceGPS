// Rôle architectural: modèle central de télémétrie partagé entre modules C++ et QML.
// Responsabilités: stocker les mesures courantes et notifier les changements via propriétés Qt.
// Dépendances principales: QObject, propriétés Q_PROPERTY et couche de signalisation Qt.

#pragma once
#include <QObject>
#include <QVariantList>

class TelemetryData : public QObject {
    Q_OBJECT
    Q_PROPERTY(double speedKmh READ speedKmh WRITE setSpeedKmh NOTIFY speedKmhChanged)
    Q_PROPERTY(bool gpsOk READ gpsOk WRITE setGpsOk NOTIFY gpsOkChanged)
    Q_PROPERTY(double lat READ lat WRITE setLat NOTIFY latChanged)
    Q_PROPERTY(double lon READ lon WRITE setLon NOTIFY lonChanged)
    Q_PROPERTY(double heading READ heading WRITE setHeading NOTIFY headingChanged)
    Q_PROPERTY(QString alertText READ alertText WRITE setAlertText NOTIFY alertTextChanged)
    Q_PROPERTY(int alertLevel READ alertLevel WRITE setAlertLevel NOTIFY alertLevelChanged)


public:
    explicit TelemetryData(QObject* parent=nullptr);

    double speedKmh() const { return m_speedKmh; }
    bool gpsOk() const { return m_gpsOk; }
    double lat() const { return m_lat; }
    double lon() const { return m_lon; }
    double heading() const { return m_heading; }
    QString alertText() const { return m_alertText; }
    int alertLevel() const { return m_alertLevel; }

public slots:
    void setSpeedKmh(double v);
    void setGpsOk(bool v);
    void setLat(double v);
    void setLon(double v);
    void setHeading(double v);
    void setAlertText(const QString& v);
    void setAlertLevel(int v);


signals:
    void speedKmhChanged();
    void gpsOkChanged();
    void latChanged();
    void lonChanged();
    void headingChanged();
    void alertTextChanged();
    void alertLevelChanged();
    void simulateRouteRequested(QVariantList routeCoords);

private:
    double m_speedKmh = 0.0;
    bool m_gpsOk = true;
    double m_lat = 48.8566;
    double m_lon = 2.3522;
    double m_heading = 0.0;
    QString m_alertText;
    int m_alertLevel = 0;

};
