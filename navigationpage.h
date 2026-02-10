#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>
#include <QQuickWidget>
#include <QStringList>
// On inclut plus QVariant ici, on passe au string simple pour la communication
#include <QJsonDocument>
#include <QJsonArray>

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

    // MODIFICATION IMPORTANTE : On reçoit une chaîne de caractères (JSON)
    void onSuggestionsReceived(const QString& jsonSuggestions);

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
};

#endif // NAVIGATIONPAGE_H
