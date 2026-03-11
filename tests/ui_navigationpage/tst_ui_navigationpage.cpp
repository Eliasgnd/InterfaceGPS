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
    NavigationPage page;

    page.onSuggestionsReceived(QStringLiteral("[\"Paris\",\"Lyon\"]"));

    auto* model = qobject_cast<QStringListModel*>(page.m_searchCompleter->model());
    QVERIFY(model != nullptr);
    QCOMPARE(model->stringList(), QStringList({"Paris", "Lyon"}));
}

void NavigationPageUiTest::onSuggestionChosen_updatesSearchField()
{
    NavigationPage page;
    auto* editSearch = page.findChild<QLineEdit*>("editSearch");
    QVERIFY(editSearch != nullptr);

    page.onSuggestionChosen("Marseille");

    QCOMPARE(editSearch->text(), QString("Marseille"));
    QVERIFY(!page.m_ignoreTextUpdate);
}

void NavigationPageUiTest::triggerSuggestionsSearch_shortQuery_doesNothingAndKeepsState()
{
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
    NavigationPage page;
    QSignalSpy routeSpy(&page, &NavigationPage::routeSearchRequested);

    page.requestRouteForText("   \t\n");

    QCOMPARE(routeSpy.count(), 0);
}

void NavigationPageUiTest::requestRouteForText_trimmedInput_emitsTrimmedDestination()
{
    NavigationPage page;
    QSignalSpy routeSpy(&page, &NavigationPage::routeSearchRequested);

    page.requestRouteForText("  Lyon  ");

    QCOMPARE(routeSpy.count(), 1);
    QCOMPARE(routeSpy.takeFirst().at(0).toString(), QString("Lyon"));
}

void NavigationPageUiTest::bindTelemetry_emitsRefreshOnBindAndTelemetryChanges()
{
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
