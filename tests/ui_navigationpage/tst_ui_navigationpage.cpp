#include <QtTest>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QStringListModel>
#include <QCompleter>

#define private public
#include "../../navigationpage.h"
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
};

void NavigationPageUiTest::constructor_wiresMainWidgetsAndDefaults()
{
    // Objectif: valider l'initialisation structurelle de NavigationPage.
    // Pourquoi: ce test attrape rapidement les régressions de wiring UI (noms d'objets, composants manquants).
    // Procédure détaillée:
    //   1) Instancier la page.
    //   2) Vérifier la présence des contrôles principaux (search, centre, zoom).
    //   3) Vérifier map.qml, le completer, le modèle de suggestions et l'intervalle du timer.
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
    // Objectif: vérifier le rendu des informations distance/temps issues du calcul d'itinéraire.
    // Pourquoi: l'utilisateur doit voir immédiatement une synthèse lisible du trajet.
    // Procédure détaillée:
    //   1) Appeler onRouteInfoReceived("12.4 km", "18 min").
    //   2) Récupérer les labels de synthèse.
    //   3) Vérifier le format final affiché côté UI.
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
    // Objectif: valider le parsing et l'injection des suggestions d'adresses.
    // Pourquoi: un modèle de completer incorrect dégrade fortement l'expérience de recherche.
    // Procédure détaillée:
    //   1) Fournir une chaîne JSON de suggestions.
    //   2) Déclencher onSuggestionsReceived.
    //   3) Vérifier le contenu exact du QStringListModel branché au completer.
    NavigationPage page;

    page.onSuggestionsReceived(QStringLiteral("[\"Paris\",\"Lyon\"]"));

    auto* model = qobject_cast<QStringListModel*>(page.m_searchCompleter->model());
    QVERIFY(model != nullptr);
    QCOMPARE(model->stringList(), QStringList({"Paris", "Lyon"}));
}

void NavigationPageUiTest::onSuggestionChosen_updatesSearchField()
{
    // Objectif: valider la sélection d'une suggestion dans le champ de recherche.
    // Pourquoi: la sélection ne doit pas laisser le composant dans un état interne incohérent.
    // Procédure détaillée:
    //   1) Appeler onSuggestionChosen("Marseille").
    //   2) Vérifier la valeur écrite dans editSearch.
    //   3) Vérifier que m_ignoreTextUpdate est revenu à false.
    NavigationPage page;
    auto* editSearch = page.findChild<QLineEdit*>("editSearch");
    QVERIFY(editSearch != nullptr);

    page.onSuggestionChosen("Marseille");

    QCOMPARE(editSearch->text(), QString("Marseille"));
    QVERIFY(!page.m_ignoreTextUpdate);
}

void NavigationPageUiTest::triggerSuggestionsSearch_shortQuery_doesNothingAndKeepsState()
{
    // Objectif: confirmer la règle de garde sur la longueur minimale de requête.
    // Pourquoi: éviter des appels réseau inutiles et du bruit de suggestions trop tôt.
    // Procédure détaillée:
    //   1) Pré-remplir le modèle avec une valeur initiale connue.
    //   2) Saisir une requête de 2 caractères.
    //   3) Appeler triggerSuggestionsSearch puis vérifier que le modèle reste inchangé.
    NavigationPage page;
    auto* editSearch = page.findChild<QLineEdit*>("editSearch");
    QVERIFY(editSearch != nullptr);

    page.onSuggestionsReceived(QStringLiteral("[\"Initial\"]"));
    editSearch->setText("ab");

    page.triggerSuggestionsSearch();

    auto* model = qobject_cast<QStringListModel*>(page.m_searchCompleter->model());
    QVERIFY(model != nullptr);
    QCOMPARE(model->stringList(), QStringList({"Initial"}));
}

QTEST_MAIN(NavigationPageUiTest)
#include "tst_ui_navigationpage.moc"
