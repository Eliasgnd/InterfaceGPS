#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>
#include <QQuickWidget>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariant>       // <-- AJOUTEZ CECI
#include <QVariantList>   // <-- AJOUTEZ CECI

namespace Ui { class NavigationPage; }
class TelemetryData;
class QCompleter;
class QStringListModel;
class QTimer;

class NavigationPage : public QWidget {
    Q_OBJECT

public:
    explicit NavigationPage(QWidget* parent = nullptr);
    ~NavigationPage();
    void bindTelemetry(TelemetryData* t);

private slots:
    void onRouteInfoReceived(const QString& distance, const QString& duration);
    void onSuggestionChosen(const QString& suggestion);
    void triggerSuggestionsSearch();
    void onSuggestionsReceived(const QString& jsonSuggestions);

    // <-- AJOUTEZ CETTE LIGNE ICI :
    void onRouteReadyForSimulation(const QVariant& pathObj);

private:
    void requestRouteForText(const QString& destination);

    Ui::NavigationPage* ui;
    TelemetryData* m_t = nullptr;
    QQuickWidget* m_mapView = nullptr;
    double m_currentZoom = 17.0;
    QCompleter* m_searchCompleter = nullptr;
    QStringListModel* m_suggestionsModel = nullptr;
    QTimer* m_suggestionDebounceTimer = nullptr;
    bool m_ignoreTextUpdate = false;

    // <-- AJOUTEZ CETTE LIGNE ICI :
    QVariantList m_lastCalculatedRoute;
};

#endif // NAVIGATIONPAGE_H
