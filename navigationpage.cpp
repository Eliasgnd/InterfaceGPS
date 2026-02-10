#include "navigationpage.h"
#include "ui_navigationpage.h"
#include "telemetrydata.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QtCore>
#include <QDebug>
#include <QListWidgetItem>

NavigationPage::NavigationPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::NavigationPage)
{
    ui->setupUi(this);

    m_mapView = new QQuickWidget(this);

    // --- 1. CONFIGURATION SUGGESTIONS ---
    ui->listSuggestions->setVisible(false);
    connect(ui->listSuggestions, &QListWidget::itemClicked, this, [this](QListWidgetItem* item){
        if (!item) {
            return;
        }
        selectSuggestion(item->text());
    });

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(1000); // 1 seconde d'attente pour économiser l'API

    connect(m_searchTimer, &QTimer::timeout, this, [this](){
        QString text = ui->editSearch->text();
        if(text.length() >= 4) {
            QMetaObject::invokeMethod(m_mapView->rootObject(), "fetchSuggestions",
                                      Q_ARG(QVariant, text));
        }
    });

    connect(ui->editSearch, &QLineEdit::textChanged, this, [this](const QString& text){
        if (text.trimmed().length() < 4) {
            ui->listSuggestions->clear();
            ui->listSuggestions->setVisible(false);
            return;
        }
        m_searchTimer->start();
    });

    // --- 2. CONFIGURATION MAPBOX ---
    QString apiKey = QString::fromLocal8Bit(qgetenv("MAPBOX_KEY"));
    m_mapView->rootContext()->setContextProperty("mapboxApiKey", apiKey);

    m_mapView->setSource(QUrl("qrc:/map.qml"));
    m_mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    connect(m_mapView, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status){
        if (status == QQuickWidget::Ready) {
            connect(m_mapView->rootObject(), SIGNAL(suggestionsReady(QVariant)),
                    this, SLOT(updateSuggestions(QVariant)));

            connect(m_mapView->rootObject(), SIGNAL(routeInfoUpdated(QString,QString)),
                    this, SLOT(onRouteInfoReceived(QString,QString)));
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
        QString destination = ui->editSearch->text();
        if(!destination.isEmpty() && m_mapView->rootObject()){
            ui->listSuggestions->setVisible(false);
            QMetaObject::invokeMethod(m_mapView->rootObject(), "searchDestination",
                                      Q_ARG(QVariant, destination));
        }
    });
}

NavigationPage::~NavigationPage(){ delete ui; }

void NavigationPage::updateSuggestions(const QVariant& suggestions) {
    m_lastSuggestions = suggestions.toStringList();

    ui->listSuggestions->clear();
    for (const QString& suggestion : m_lastSuggestions) {
        if (!suggestion.trimmed().isEmpty()) {
            ui->listSuggestions->addItem(suggestion);
        }
    }

    ui->listSuggestions->setVisible(ui->listSuggestions->count() > 0);
}

void NavigationPage::selectSuggestion(const QString& suggestion)
{
    if (suggestion.isEmpty() || !m_mapView || !m_mapView->rootObject()) {
        return;
    }

    ui->editSearch->setText(suggestion);
    ui->listSuggestions->setVisible(false);
    QMetaObject::invokeMethod(m_mapView->rootObject(), "searchDestination",
                              Q_ARG(QVariant, suggestion));
}

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
