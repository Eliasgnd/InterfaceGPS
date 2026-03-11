#include <QtTest>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QStringListModel>
#include <QCompleter>
#include <QSignalSpy>

#define private public
#include "../../navigationpage.h"
#include "../../telemetrydata.h"
#undef private

class NavigationPageUiTest : public QObject
{
    Q_OBJECT

private slots:
    void constructor_wiresMainWidgetsAndDefaults();
    void onRouteInfoReceived_updatesInfoLabels();
    void onSuggestionsReceived_updatesCompleterModel();
    void onSuggestionChosen_updatesSearchField();
    void triggerSuggestionsSearch_shortQuery_doesNothingAndKeepsState();
    void requestRouteForText_emptyInput_emitsNothing();
    void requestRouteForText_trimmedInput_emitsTrimmedDestination();
    void bindTelemetry_emitsRefreshOnBindAndTelemetryChanges();
};

void NavigationPageUiTest::constructor_wiresMainWidgetsAndDefaults()
{
    // Objectif: valider le montage initial de l'écran de navigation.
    // Pourquoi: si un widget clé manque, l'utilisateur ne peut ni chercher ni piloter la carte.
    // Procédure détaillée:
    //   1) Instancier NavigationPage.
    //   2) Vérifier la présence des contrôles principaux (recherche, centrage, zoom).
    //   3) Vérifier la source QML, le completer et le timer de debounce (800 ms).
    NavigationPage page;

    QVERIFY(page.findChild<QLineEdit*>("editSearch") != nullptr);
    QVERIFY(page.findChild<QPushButton*>("btnSearch") != nullptr);
    QVERIFY(page.findChild<QPushButton*>("btnCenter") != nullptr);
    QVERIFY(page.findChild<QPushButton*>("btnZoomIn") != nullptr);
    QVERIFY(page.findChild<QPushButton*>("btnZoomOut") != nullptr);

    QVERIFY(page.m_mapView != nullptr);
    QCOMPARE(page.m_mapView->source(), QUrl(QStringLiteral("qrc:/map.qml")));
    QVERIFY(page.m_searchCompleter != nullptr);
    QVERIFY(page.m_suggestionsModel != nullptr);
    QCOMPARE(page.m_suggestionDebounceTimer->interval(), 800);
}

void NavigationPageUiTest::onRouteInfoReceived_updatesInfoLabels()
{
    // Objectif: confirmer la mise à jour des informations de trajet dans l'UI.
    // Pourquoi: distance et durée sont des retours essentiels après calcul d'itinéraire.
    // Procédure détaillée:
    //   1) Injecter une distance et un temps simulés.
    //   2) Récupérer les labels d'affichage.
    //   3) Vérifier leur texte formaté final.
    NavigationPage page;

    page.onRouteInfoReceived("12.4 km", "18 min");

    auto* lblLat = page.findChild<QLabel*>("lblLat");
    auto* lblLon = page.findChild<QLabel*>("lblLon");
    QVERIFY(lblLat != nullptr);
    QVERIFY(lblLon != nullptr);
    QCOMPARE(lblLat->text(), QString("Dist: 12.4 km"));
    QCOMPARE(lblLon->text(), QString("Temps: 18 min"));
}

void NavigationPageUiTest::onSuggestionsReceived_updatesCompleterModel()
{
    // Objectif: vérifier l'alimentation des suggestions d'auto-complétion.
    // Pourquoi: l'assistance à la saisie dépend du modèle du completer.
    // Procédure détaillée:
    //   1) Envoyer un JSON de suggestions (Paris, Lyon).
    //   2) Lire le QStringListModel branché au completer.
    //   3) Vérifier que la liste interne correspond exactement aux suggestions reçues.
    NavigationPage page;

    page.onSuggestionsReceived(QStringLiteral("[\"Paris\",\"Lyon\"]"));

    auto* model = qobject_cast<QStringListModel*>(page.m_searchCompleter->model());
    QVERIFY(model != nullptr);
    QCOMPARE(model->stringList(), QStringList({"Paris", "Lyon"}));
}

void NavigationPageUiTest::onSuggestionChosen_updatesSearchField()
{
    // Objectif: valider le report d'une suggestion choisie vers le champ de recherche.
    // Pourquoi: cliquer une suggestion doit préparer immédiatement une recherche route.
    // Procédure détaillée:
    //   1) Appeler onSuggestionChosen("Marseille").
    //   2) Vérifier que le texte du QLineEdit est mis à jour.
    //   3) Vérifier que le garde-fou m_ignoreTextUpdate revient à false.
    NavigationPage page;
    auto* editSearch = page.findChild<QLineEdit*>("editSearch");
    QVERIFY(editSearch != nullptr);

    page.onSuggestionChosen("Marseille");

    QCOMPARE(editSearch->text(), QString("Marseille"));
    QVERIFY(!page.m_ignoreTextUpdate);
}

void NavigationPageUiTest::triggerSuggestionsSearch_shortQuery_doesNothingAndKeepsState()
{
    // Objectif: s'assurer qu'une requête trop courte ne déclenche pas de recherche suggestions.
    // Pourquoi: cela évite du bruit réseau et des résultats peu pertinents.
    // Procédure détaillée:
    //   1) Précharger le modèle avec une valeur "Initial".
    //   2) Mettre un texte court ("ab").
    //   3) Déclencher triggerSuggestionsSearch().
    //   4) Vérifier: aucun signal d'émission et modèle inchangé.
    NavigationPage page;
    auto* editSearch = page.findChild<QLineEdit*>("editSearch");
    QVERIFY(editSearch != nullptr);

    QSignalSpy suggestionsSpy(&page, &NavigationPage::suggestionsSearchRequested);

    page.onSuggestionsReceived(QStringLiteral("[\"Initial\"]"));
    editSearch->setText("ab");

    page.triggerSuggestionsSearch();

    auto* model = qobject_cast<QStringListModel*>(page.m_searchCompleter->model());
    QVERIFY(model != nullptr);
    QCOMPARE(model->stringList(), QStringList({"Initial"}));
    QCOMPARE(suggestionsSpy.count(), 0);
}

void NavigationPageUiTest::requestRouteForText_emptyInput_emitsNothing()
{
    // Objectif: empêcher une recherche itinéraire vide.
    // Pourquoi: lancer une requête sans destination n'a pas de sens fonctionnel.
    // Procédure détaillée:
    //   1) Observer routeSearchRequested avec QSignalSpy.
    //   2) Fournir une chaîne vide/blanche.
    //   3) Vérifier qu'aucun signal n'est émis.
    NavigationPage page;
    QSignalSpy routeSpy(&page, &NavigationPage::routeSearchRequested);

    page.requestRouteForText("   \t\n");

    QCOMPARE(routeSpy.count(), 0);
}

void NavigationPageUiTest::requestRouteForText_trimmedInput_emitsTrimmedDestination()
{
    // Objectif: vérifier le nettoyage de saisie avant émission de la destination.
    // Pourquoi: le backend doit recevoir une valeur exploitable sans espaces parasites.
    // Procédure détaillée:
    //   1) Observer routeSearchRequested.
    //   2) Appeler requestRouteForText avec espaces autour du mot.
    //   3) Vérifier une seule émission avec la valeur trimée ("Lyon").
    NavigationPage page;
    QSignalSpy routeSpy(&page, &NavigationPage::routeSearchRequested);

    page.requestRouteForText("  Lyon  ");

    QCOMPARE(routeSpy.count(), 1);
    QCOMPARE(routeSpy.takeFirst().at(0).toString(), QString("Lyon"));
}

void NavigationPageUiTest::bindTelemetry_emitsRefreshOnBindAndTelemetryChanges()
{
    // Objectif: valider le pont entre TelemetryData et rafraîchissement de la carte.
    // Pourquoi: position, cap et vitesse doivent remonter en direct côté navigation.
    // Procédure détaillée:
    //   1) Binder TelemetryData au composant et vérifier un refresh initial.
    //   2) Modifier lat/lon/heading/speed dans TelemetryData.
    //   3) Vérifier plusieurs émissions puis contrôler les dernières valeurs transmises.
    NavigationPage page;
    TelemetryData data;

    QSignalSpy refreshSpy(&page, &NavigationPage::telemetryRefreshRequested);
    page.bindTelemetry(&data);

    QVERIFY(refreshSpy.count() >= 1); // refresh initial

    data.setLat(43.7);
    data.setLon(7.2);
    data.setHeading(91.0);
    data.setSpeedKmh(55.5);

    QVERIFY(refreshSpy.count() >= 5);

    const auto lastArgs = refreshSpy.last();
    QCOMPARE(lastArgs.at(0).toDouble(), 43.7);
    QCOMPARE(lastArgs.at(1).toDouble(), 7.2);
    QCOMPARE(lastArgs.at(2).toDouble(), 91.0);
    QCOMPARE(lastArgs.at(3).toDouble(), 55.5);
}

QTEST_MAIN(NavigationPageUiTest)
#include "tst_ui_navigationpage.moc"
