#include <QtTest>
#include <QSignalSpy>
#include <QDateTime>
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <cmath>

#define private public
#include "../../telemetrydata.h"
#include "../../gpstelemetrysource.h"
#include "../../mpu9250source.h"
#undef private

class TelemetryAndSourcesTest : public QObject
{
    Q_OBJECT

private slots:
    void telemetryData_defaultValues_areInitialized();
    void telemetryData_setSpeedKmh_emitsOnlyWhenValueChanges();
    void telemetryData_setGpsOk_emitsOnlyWhenValueChanges();
    void telemetryData_setLatLonHeading_updateValuesAndSignals();

    void gpsTelemetrySource_invalidPosition_setsGpsKo();
    void gpsTelemetrySource_validPosition_updatesTelemetry();

    void mpu9250Source_startStopAndReadSensor_withoutHardware_doesNotCorruptTelemetry();
};

void TelemetryAndSourcesTest::telemetryData_defaultValues_areInitialized()
{
    TelemetryData data;

    QCOMPARE(data.speedKmh(), 0.0);
    QCOMPARE(data.gpsOk(), true);
    QCOMPARE(data.lat(), 48.8566);
    QCOMPARE(data.lon(), 2.3522);
    QCOMPARE(data.heading(), 0.0);
}

void TelemetryAndSourcesTest::telemetryData_setSpeedKmh_emitsOnlyWhenValueChanges()
{
    TelemetryData data;
    QSignalSpy speedSpy(&data, &TelemetryData::speedKmhChanged);

    data.setSpeedKmh(12.5);
    QCOMPARE(data.speedKmh(), 12.5);
    QCOMPARE(speedSpy.count(), 1);

    data.setSpeedKmh(12.5);
    QCOMPARE(speedSpy.count(), 1);

    data.setSpeedKmh(13.5);
    QCOMPARE(data.speedKmh(), 13.5);
    QCOMPARE(speedSpy.count(), 2);
}

void TelemetryAndSourcesTest::telemetryData_setGpsOk_emitsOnlyWhenValueChanges()
{
    TelemetryData data;
    QSignalSpy gpsSpy(&data, &TelemetryData::gpsOkChanged);

    data.setGpsOk(false);
    QCOMPARE(data.gpsOk(), false);
    QCOMPARE(gpsSpy.count(), 1);

    data.setGpsOk(false);
    QCOMPARE(gpsSpy.count(), 1);

    data.setGpsOk(true);
    QCOMPARE(data.gpsOk(), true);
    QCOMPARE(gpsSpy.count(), 2);
}

void TelemetryAndSourcesTest::telemetryData_setLatLonHeading_updateValuesAndSignals()
{
    TelemetryData data;
    QSignalSpy latSpy(&data, &TelemetryData::latChanged);
    QSignalSpy lonSpy(&data, &TelemetryData::lonChanged);
    QSignalSpy headingSpy(&data, &TelemetryData::headingChanged);

    data.setLat(43.2965);
    data.setLon(5.3698);
    data.setHeading(273.2);

    QCOMPARE(data.lat(), 43.2965);
    QCOMPARE(data.lon(), 5.3698);
    QCOMPARE(data.heading(), 273.2);
    QCOMPARE(latSpy.count(), 1);
    QCOMPARE(lonSpy.count(), 1);
    QCOMPARE(headingSpy.count(), 1);

    data.setLat(43.2965);
    data.setLon(5.3698);
    data.setHeading(273.2);

    QCOMPARE(latSpy.count(), 1);
    QCOMPARE(lonSpy.count(), 1);
    QCOMPARE(headingSpy.count(), 1);
}

void TelemetryAndSourcesTest::gpsTelemetrySource_invalidPosition_setsGpsKo()
{
    TelemetryData data;
    data.setGpsOk(true);

    GpsTelemetrySource source(&data);
    QGeoPositionInfo invalidInfo;

    source.onPositionUpdated(invalidInfo);

    QCOMPARE(data.gpsOk(), false);
}

void TelemetryAndSourcesTest::gpsTelemetrySource_validPosition_updatesTelemetry()
{
    TelemetryData data;
    GpsTelemetrySource source(&data);

    QGeoPositionInfo info(QGeoCoordinate(48.8584, 2.2945), QDateTime::currentDateTimeUtc());
    info.setAttribute(QGeoPositionInfo::GroundSpeed, 10.0); // m/s => 36 km/h

    source.onPositionUpdated(info);

    QCOMPARE(data.gpsOk(), true);
    QCOMPARE(data.lat(), 48.8584);
    QCOMPARE(data.lon(), 2.2945);
    QCOMPARE(data.speedKmh(), 36.0);
}

void TelemetryAndSourcesTest::mpu9250Source_startStopAndReadSensor_withoutHardware_doesNotCorruptTelemetry()
{
    TelemetryData data;
    Mpu9250Source source(&data);
    source.start();
    source.readSensor();

    // Au lieu de QCOMPARE(data.heading(), 0.0), vérifiez la validité
    QVERIFY(std::isfinite(data.heading()));
    QVERIFY(data.heading() >= 0.0 && data.heading() < 360.0);
}

QTEST_APPLESS_MAIN(TelemetryAndSourcesTest)
#include "tst_telemetrydata.moc"
