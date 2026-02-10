#include "navigationpage.h"
#include "ui_navigationpage.h"
#include "telemetrydata.h"
#include <QCompleter>
#include <QStringListModel>
#include <QTimer>
#include <QQmlContext>
#include <QQuickItem>
#include <QtCore>

NavigationPage::NavigationPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::NavigationPage)
{
    ui->setupUi(this);

    m_suggestionsModel = new QStringListModel(this);
    m_searchCompleter = new QCompleter(m_suggestionsModel, this);
    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_searchCompleter->setFilterMode(Qt::MatchContains);
    m_searchCompleter->setCompletionMode(QCompleter::PopupCompletion);
    ui->editSearch->setCompleter(m_searchCompleter);

    m_suggestionDebounceTimer = new QTimer(this);
    m_suggestionDebounceTimer->setSingleShot(true);
    m_suggestionDebounceTimer->setInterval(250);

    connect(m_suggestionDebounceTimer, &QTimer::timeout,
            this, &NavigationPage::triggerSuggestionsSearch);
    connect(m_searchCompleter, &QCompleter::activated,
            this, &NavigationPage::onSuggestionChosen);
    connect(ui->editSearch, &QLineEdit::textEdited, this, [this]() {
        if (m_ignoreTextUpdate) {
            return;
        }
        m_suggestionDebounceTimer->start();
    });

    m_mapView = new QQuickWidget(this);

    // --- 2. CONFIGURATION MAPBOX ---
    QString apiKey = QString::fromLocal8Bit(qgetenv("MAPBOX_KEY"));
    m_mapView->rootContext()->setContextProperty("mapboxApiKey", apiKey);

    m_mapView->setSource(QUrl("qrc:/map.qml"));
    m_mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    connect(m_mapView, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status){
        if (status == QQuickWidget::Ready) {
            connect(m_mapView->rootObject(), SIGNAL(routeInfoUpdated(QString,QString)),
                    this, SLOT(onRouteInfoReceived(QString,QString)));
            connect(m_mapView->rootObject(), SIGNAL(suggestionsUpdated(QVariantList)),
                    this, [this](const QVariantList& rawSuggestions) {
                        QStringList suggestions;
                        suggestions.reserve(rawSuggestions.size());
                        for (const QVariant& suggestion : rawSuggestions) {
                            const QString value = suggestion.toString().trimmed();
                            if (!value.isEmpty() && !suggestions.contains(value)) {
                                suggestions << value;
                            }
                        }
                        m_suggestionsModel->setStringList(suggestions);

                        if (!suggestions.isEmpty() && ui->editSearch->hasFocus()) {
                            m_searchCompleter->complete();
                        }
                    });
        }
    });

    ui->lblMap->setVisible(false);
    ui->mapLayout->addWidget(m_mapView);

    // --- 3. ACTIONS BOUTONS ---
    connect(ui->btnZoomIn, &QPushButton::clicked, this, [this](){
        m_currentZoom = qMin(20.0, m_currentZoom + 1.0);
        if(m_mapView->rootObject()) m_mapView->rootObject()->setProperty("carZoom", m_currentZoom);
    });

    connect(ui->btnZoomOut, &QPushButton::clicked, this, [this](){
        m_currentZoom = qMax(2.0, m_currentZoom - 1.0);
        if(m_mapView->rootObject()) m_mapView->rootObject()->setProperty("carZoom", m_currentZoom);
    });

    connect(ui->btnCenter, &QPushButton::clicked, this, [this](){
        if(m_mapView && m_mapView->rootObject()){
            QMetaObject::invokeMethod(m_mapView->rootObject(), "recenterMap");
        }
    });

    connect(ui->btnSearch, &QPushButton::clicked, this, [this](){
        requestRouteForText(ui->editSearch->text());
    });
}

NavigationPage::~NavigationPage(){ delete ui; }

void NavigationPage::onRouteInfoReceived(const QString& distance, const QString& duration) {
    ui->lblLat->setText("Dist: " + distance);
    ui->lblLon->setText("Temps: " + duration);
}

void NavigationPage::bindTelemetry(TelemetryData* t)
{
    m_t = t;
    if(!m_t) return;

    auto refresh = [this](){
        if(m_mapView && m_mapView->rootObject()){
            m_mapView->rootObject()->setProperty("carLat", m_t->lat());
            m_mapView->rootObject()->setProperty("carLon", m_t->lon());
            m_mapView->rootObject()->setProperty("carHeading", m_t->heading());
        }
    };
    refresh();
    connect(m_t, &TelemetryData::latChanged, this, refresh);
    connect(m_t, &TelemetryData::lonChanged, this, refresh);
    connect(m_t, &TelemetryData::headingChanged, this, refresh);
}

void NavigationPage::requestRouteForText(const QString& destination)
{
    if (destination.trimmed().isEmpty() || !m_mapView || !m_mapView->rootObject()) {
        return;
    }

    QMetaObject::invokeMethod(m_mapView->rootObject(), "searchDestination",
                              Q_ARG(QVariant, destination.trimmed()));
}

void NavigationPage::onSuggestionChosen(const QString& suggestion)
{
    m_ignoreTextUpdate = true;
    ui->editSearch->setText(suggestion);
    m_ignoreTextUpdate = false;
    requestRouteForText(suggestion);
}

void NavigationPage::triggerSuggestionsSearch()
{
    const QString query = ui->editSearch->text().trimmed();
    if (query.size() < 2) {
        m_suggestionsModel->setStringList({});
        return;
    }

    if (!m_mapView || !m_mapView->rootObject()) {
        return;
    }

    QMetaObject::invokeMethod(m_mapView->rootObject(), "requestSuggestions",
                              Q_ARG(QVariant, query));
}
