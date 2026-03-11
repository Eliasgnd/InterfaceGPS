#include <QtTest>
#include <QSignalSpy>
#include <QDateTime>
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <limits>
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
    void telemetryData_setters_withNanAndInf_storeAndNotify();

    void gpsTelemetrySource_invalidPosition_setsGpsKo();
    void gpsTelemetrySource_validPosition_updatesTelemetry();
    void gpsTelemetrySource_validPosition_withoutGroundSpeed_keepsPreviousSpeed();
    void gpsTelemetrySource_validPosition_withDirection_doesNotChangeHeadingYet();

    void mpu9250Source_startStopAndReadSensor_withoutHardware_doesNotCorruptTelemetry();
};

void TelemetryAndSourcesTest::telemetryData_defaultValues_areInitialized()
{
    // Objectif: vérifier les valeurs initiales quand aucune donnée capteur n'a encore été reçue.
    // Pourquoi: ces valeurs servent de base d'affichage au démarrage de l'application.
    // Procédure détaillée:
    //   1) Créer un objet TelemetryData neuf.
    //   2) Lire chaque propriété publique (vitesse, état GPS, latitude, longitude, cap).
    //   3) Comparer avec les valeurs par défaut attendues.
    TelemetryData data;

    QCOMPARE(data.speedKmh(), 0.0);
    QCOMPARE(data.gpsOk(), true);
    QCOMPARE(data.lat(), 48.8566);
    QCOMPARE(data.lon(), 2.3522);
    QCOMPARE(data.heading(), 0.0);
}

void TelemetryAndSourcesTest::telemetryData_setSpeedKmh_emitsOnlyWhenValueChanges()
{
    // Objectif: contrôler la logique "anti-bruit" des signaux Qt sur la vitesse.
    // Pourquoi: réémettre un signal avec la même valeur ferait des mises à jour UI inutiles.
    // Procédure détaillée:
    //   1) Observer speedKmhChanged avec QSignalSpy.
    //   2) Écrire 12.5 km/h puis vérifier 1 émission.
    //   3) Réécrire 12.5 km/h: aucun signal supplémentaire.
    //   4) Écrire 13.5 km/h: une deuxième émission est attendue.
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
    // Objectif: valider le même comportement pour l'état GPS (booléen).
    // Pourquoi: l'UI (icône GPS) ne doit changer que lors d'une vraie transition d'état.
    // Procédure détaillée:
    //   1) Observer gpsOkChanged.
    //   2) Passer true -> false puis vérifier l'émission.
    //   3) Réappliquer false (pas de nouvelle émission), puis false -> true (2e émission).
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
    // Objectif: vérifier simultanément stockage des coordonnées/cap + émission des signaux dédiés.
    // Pourquoi: la carte et la flèche de direction dépendent directement de ces trois champs.
    // Procédure détaillée:
    //   1) Attacher trois QSignalSpy (lat/lon/heading).
    //   2) Écrire de nouvelles valeurs puis vérifier données + 1 signal chacun.
    //   3) Réécrire les mêmes valeurs et vérifier qu'aucun signal superflu n'est émis.
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

void TelemetryAndSourcesTest::telemetryData_setters_withNanAndInf_storeAndNotify()
{
    // Objectif: tester les valeurs numériques extrêmes (Infini, NaN).
    // Pourquoi: des capteurs réels peuvent produire des données invalides; le modèle doit rester stable.
    // Procédure détaillée:
    //   1) Injecter +Inf en vitesse et vérifier stockage + signal.
    //   2) Injecter NaN en latitude et vérifier stockage + signal.
    //   3) Injecter -Inf en longitude et vérifier stockage, signe négatif et signal.
    TelemetryData data;
    QSignalSpy speedSpy(&data, &TelemetryData::speedKmhChanged);
    QSignalSpy latSpy(&data, &TelemetryData::latChanged);
    QSignalSpy lonSpy(&data, &TelemetryData::lonChanged);

    data.setSpeedKmh(std::numeric_limits<double>::infinity());
    QVERIFY(std::isinf(data.speedKmh()));
    QCOMPARE(speedSpy.count(), 1);

    data.setLat(std::numeric_limits<double>::quiet_NaN());
    QVERIFY(std::isnan(data.lat()));
    QCOMPARE(latSpy.count(), 1);

    data.setLon(-std::numeric_limits<double>::infinity());
    QVERIFY(std::isinf(data.lon()));
    QVERIFY(data.lon() < 0.0);
    QCOMPARE(lonSpy.count(), 1);
}

void TelemetryAndSourcesTest::gpsTelemetrySource_invalidPosition_setsGpsKo()
{
    // Objectif: valider la réaction à une position GPS invalide.
    // Pourquoi: l'interface doit prévenir immédiatement que la géolocalisation n'est pas fiable.
    // Procédure détaillée:
    //   1) Créer un QGeoPositionInfo vide (invalide).
    //   2) Appeler onPositionUpdated.
    //   3) Vérifier que gpsOk passe à false.
    TelemetryData data;
    data.setGpsOk(true);

    GpsTelemetrySource source(&data);
    QGeoPositionInfo invalidInfo;

    source.onPositionUpdated(invalidInfo);

    QCOMPARE(data.gpsOk(), false);
}

void TelemetryAndSourcesTest::gpsTelemetrySource_validPosition_updatesTelemetry()
{
    // Objectif: vérifier le flux nominal GPS -> TelemetryData.
    // Pourquoi: c'est le chemin principal utilisé en navigation.
    // Procédure détaillée:
    //   1) Construire une position valide (lat/lon).
    //   2) Ajouter GroundSpeed=10 m/s.
    //   3) Vérifier gpsOk=true, coordonnées mises à jour et conversion vitesse 10*3.6=36 km/h.
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

void TelemetryAndSourcesTest::gpsTelemetrySource_validPosition_withoutGroundSpeed_keepsPreviousSpeed()
{
    // Objectif: confirmer que l'absence de vitesse GPS ne remet pas la vitesse à zéro.
    // Pourquoi: conserver la dernière valeur évite des "sauts" visuels trompeurs.
    // Procédure détaillée:
    //   1) Initialiser speedKmh à 42.
    //   2) Envoyer une position valide sans attribut GroundSpeed.
    //   3) Vérifier que seule la position est mise à jour, pas la vitesse.
    TelemetryData data;
    data.setSpeedKmh(42.0);

    GpsTelemetrySource source(&data);
    QGeoPositionInfo info(QGeoCoordinate(45.7640, 4.8357), QDateTime::currentDateTimeUtc());

    source.onPositionUpdated(info);

    QCOMPARE(data.gpsOk(), true);
    QCOMPARE(data.lat(), 45.7640);
    QCOMPARE(data.lon(), 4.8357);
    QCOMPARE(data.speedKmh(), 42.0);
}

void TelemetryAndSourcesTest::gpsTelemetrySource_validPosition_withDirection_doesNotChangeHeadingYet()
{
    // Objectif: figer le comportement actuel sur l'attribut Direction.
    // Pourquoi: la lecture existe, mais la mise à jour de heading est volontairement non activée.
    // Procédure détaillée:
    //   1) Définir un cap initial (12°).
    //   2) Envoyer une position avec Direction=198°.
    //   3) Vérifier que heading reste à 12° tant que la feature n'est pas implémentée.
    TelemetryData data;
    data.setHeading(12.0);

    GpsTelemetrySource source(&data);
    QGeoPositionInfo info(QGeoCoordinate(47.2184, -1.5536), QDateTime::currentDateTimeUtc());
    info.setAttribute(QGeoPositionInfo::Direction, 198.0);
    info.setAttribute(QGeoPositionInfo::GroundSpeed, 15.0);

    source.onPositionUpdated(info);

    // Le code lit Direction mais n'applique pas encore setHeading(course) (bloc commenté).
    QCOMPARE(data.heading(), 12.0);
}

void TelemetryAndSourcesTest::mpu9250Source_startStopAndReadSensor_withoutHardware_doesNotCorruptTelemetry()
{
    // Objectif: vérifier la robustesse du capteur inertiel en environnement sans matériel réel.
    // Pourquoi: les tests CI ou postes dev n'ont généralement pas de MPU9250 branché.
    // Procédure détaillée:
    //   1) Démarrer la source MPU puis forcer une lecture.
    //   2) Vérifier que heading reste un nombre fini.
    //   3) Vérifier la plage normalisée [0, 360).
    TelemetryData data;
    Mpu9250Source source(&data);
    source.start();
    source.readSensor();

    QVERIFY(std::isfinite(data.heading()));
    QVERIFY(data.heading() >= 0.0 && data.heading() < 360.0);
}

QTEST_APPLESS_MAIN(TelemetryAndSourcesTest)
#include "tst_telemetrydata.moc"
