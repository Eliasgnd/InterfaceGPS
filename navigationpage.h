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
    void onRouteInfoReceived(const QString& distance, const QString& duration);

    // CHANGEZ CETTE LIGNE : remplacez QStringList par QVariant
    void updateSuggestions(const QVariant& suggestions);

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
