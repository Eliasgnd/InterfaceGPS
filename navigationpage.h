#pragma once
#include <QWidget>
#include <QQuickWidget>
#include <QTimer>            // Indispensable pour le délai
#include <QStringListModel>  // Pour la liste de suggestions
#include <QCompleter>        // Pour le menu déroulant

namespace Ui { class NavigationPage; }
class TelemetryData;

class NavigationPage : public QWidget {
    Q_OBJECT
public:
    explicit NavigationPage(QWidget* parent = nullptr);
    ~NavigationPage();
    void bindTelemetry(TelemetryData* t);

private slots:
    // Reçoit la distance et le temps (ex: "12 km", "15 min")
    void onRouteInfoReceived(const QString& distance, const QString& duration);

    // Reçoit la liste des villes suggérées (ex: "Paris", "Pariana", "Paris, TX")
    void updateSuggestions(const QStringList& suggestions);

private:
    Ui::NavigationPage* ui;
    TelemetryData* m_t = nullptr;
    QQuickWidget* m_mapView = nullptr;
    double m_currentZoom = 17.0;

    // --- Les variables pour l'auto-complétion ---
    QTimer* m_searchTimer = nullptr;
    QCompleter* m_completer = nullptr;
    QStringListModel* m_suggestModel = nullptr;
};
