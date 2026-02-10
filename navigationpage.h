#pragma once
#include <QWidget>
#include <QQuickWidget>
#include <QTimer>            // Indispensable pour le délai
#include <QStringList>       // Pour stocker les suggestions
#include <QEvent>

namespace Ui { class NavigationPage; }
class TelemetryData;

class NavigationPage : public QWidget {
    Q_OBJECT
public:
    explicit NavigationPage(QWidget* parent = nullptr);
    ~NavigationPage();
    void bindTelemetry(TelemetryData* t);

private slots:
    void onRouteInfoReceived(const QString& distance, const QString& duration);

    void updateSuggestions(const QVariant& suggestions);

private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void updateSuggestionsGeometry();
    void selectSuggestion(const QString& suggestion);

    Ui::NavigationPage* ui;
    TelemetryData* m_t = nullptr;
    QQuickWidget* m_mapView = nullptr;
    double m_currentZoom = 17.0;

    // --- Variables recherche ---
    QTimer* m_searchTimer = nullptr;
    QStringList m_lastSuggestions;
};
