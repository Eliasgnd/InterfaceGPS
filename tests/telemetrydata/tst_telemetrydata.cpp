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
    // Objectif: valider le contrat d'initialisation de TelemetryData.
    // Ce test protège contre les régressions de valeurs par défaut qui peuvent casser
    // l'affichage initial de l'UI (vitesse, statut GPS, position de départ, cap).
    // Procédure détaillée:
    //   1) Créer une instance fraîche de TelemetryData.
    //   2) Lire chaque propriété publique via ses getters.
    //   3) Comparer avec les valeurs de référence attendues.
    TelemetryData data;

    QCOMPARE(data.speedKmh(), 0.0);
    QCOMPARE(data.gpsOk(), true);
    QCOMPARE(data.lat(), 48.8566);
    QCOMPARE(data.lon(), 2.3522);
    QCOMPARE(data.heading(), 0.0);
}

void TelemetryAndSourcesTest::telemetryData_setSpeedKmh_emitsOnlyWhenValueChanges()
{
    // Objectif: garantir que speedKmhChanged suit une logique change-only.
    // Pourquoi: éviter des notifications inutiles (rafraîchissements UI superflus).
    // Procédure détaillée:
    //   1) Brancher un QSignalSpy sur speedKmhChanged.
    //   2) Écrire 12.5 -> 1 émission attendue.
    //   3) Réécrire 12.5 -> aucune émission supplémentaire.
    //   4) Écrire 13.5 -> seconde émission attendue.
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
    // Objectif: vérifier que gpsOkChanged est émis uniquement lors d'un vrai changement d'état.
    // Pourquoi: ce signal pilote souvent des indicateurs critiques (icônes/alertes GPS).
    // Procédure détaillée:
    //   1) Observer gpsOkChanged avec QSignalSpy.
    //   2) Passer à false -> 1 émission.
    //   3) Repasser à false -> toujours 1 émission (pas de bruit).
    //   4) Revenir à true -> 2e émission.
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
    // Objectif: valider la cohérence complète des setters lat/lon/heading.
    // Pourquoi: ces propriétés alimentent la carte, la navigation et les widgets de cap.
    // Procédure détaillée:
    //   1) Connecter 3 QSignalSpy (latChanged, lonChanged, headingChanged).
    //   2) Écrire un triplet de nouvelles valeurs, puis vérifier stockage + 1 émission/signal.
    //   3) Réécrire exactement les mêmes valeurs pour confirmer absence de nouvelle émission.
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
    // Objectif: valider le comportement de repli en cas de trame GPS invalide.
    // Pourquoi: éviter de conserver un faux état GPS OK quand les données sont corrompues/absentes.
    // Procédure détaillée:
    //   1) Partir d'un état gpsOk=true.
    //   2) Injecter un QGeoPositionInfo non valide via la source GPS.
    //   3) Vérifier que gpsOk est forcé à false.
    TelemetryData data;
    data.setGpsOk(true);

    GpsTelemetrySource source(&data);
    QGeoPositionInfo invalidInfo;

    source.onPositionUpdated(invalidInfo);

    QCOMPARE(data.gpsOk(), false);
}

void TelemetryAndSourcesTest::gpsTelemetrySource_validPosition_updatesTelemetry()
{
    // Objectif: vérifier le chemin nominal de mise à jour GPS -> télémétrie.
    // Pourquoi: garantir la bonne propagation des coordonnées et de la vitesse convertie.
    // Procédure détaillée:
    //   1) Construire une position valide (coordonnées + timestamp).
    //   2) Injecter GroundSpeed=10 m/s.
    //   3) Appeler onPositionUpdated puis vérifier gpsOk, lat/lon et vitesse à 36 km/h.
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
    // Objectif: tester la robustesse du chemin capteur MPU9250 en environnement sans hardware.
    // Pourquoi: les CI n'ont pas de capteur physique, mais le code ne doit ni planter ni corrompre les données.
    // Procédure détaillée:
    //   1) Démarrer la source et forcer une lecture.
    //   2) Vérifier que heading reste une valeur numérique finie.
    //   3) Vérifier que la normalisation d'angle reste dans [0, 360).
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
