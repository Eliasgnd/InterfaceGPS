#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>
#include <QQuickWidget>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QEvent>

namespace Ui { class NavigationPage; }
class TelemetryData;
class QCompleter;
class QStringListModel;
class QTimer;
class Clavier; // <--- 1. AJOUTE CETTE LIGNE ICI (Forward Declaration)

class NavigationPage : public QWidget {
    Q_OBJECT

public:
    explicit NavigationPage(QWidget* parent = nullptr);
    ~NavigationPage();
    void bindTelemetry(TelemetryData* t);
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onRouteInfoReceived(const QString& distance, const QString& duration);
    void onSuggestionChosen(const QString& suggestion);
    void triggerSuggestionsSearch();
    void onSuggestionsReceived(const QString& jsonSuggestions);
    void onRouteReadyForSimulation(const QVariant& pathObj);

private:
    void requestRouteForText(const QString& destination);
    void openVirtualKeyboard();

    Ui::NavigationPage* ui;
    TelemetryData* m_t = nullptr;
    QQuickWidget* m_mapView = nullptr;
    QCompleter* m_searchCompleter = nullptr;
    QStringListModel* m_suggestionsModel = nullptr;
    QTimer* m_suggestionDebounceTimer = nullptr;
    bool m_ignoreTextUpdate = false;
    QVariantList m_lastCalculatedRoute;

    // 2. VÉRIFIE QUE CETTE LIGNE EST BIEN ÉCRITE (Erreur C2143 venait d'ici)
    Clavier* m_currentClavier = nullptr;
};

#endif // NAVIGATIONPAGE_H
