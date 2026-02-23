// Rôle architectural: façade widget de la navigation cartographique.
// Responsabilités: relier la vue QML de carte, la recherche d'adresses et le clavier virtuel.
// Dépendances principales: QWidget, QQuickWidget, Clavier, TelemetryData et services HTTP Mapbox.

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
class Clavier;

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

    Clavier* m_currentClavier = nullptr;
};

#endif
