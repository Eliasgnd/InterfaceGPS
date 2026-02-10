#pragma once
#include <QWidget>
#include <QQuickWidget>
#include <QTimer>            // Indispensable pour le délai
#include <QStringList>       // Pour stocker les suggestions

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

    // CHANGEZ CETTE LIGNE : remplacez QStringList par QVariant
    void updateSuggestions(const QVariant& suggestions);

private:
    void selectSuggestion(const QString& suggestion);

    Ui::NavigationPage* ui;
    TelemetryData* m_t = nullptr;
    QQuickWidget* m_mapView = nullptr;
    double m_currentZoom = 17.0;

    // --- Variables recherche ---
    QTimer* m_searchTimer = nullptr;
    QStringList m_lastSuggestions;
};
