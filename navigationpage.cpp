#include "navigationpage.h"
#include "ui_navigationpage.h"
#include "telemetrydata.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QtCore>
#include <QDebug>
#include <QCompleter>
#include <QAbstractItemView> // <--- LA LIGNE INDISPENSABLE QUI RÉPARE L'ERREUR

NavigationPage::NavigationPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::NavigationPage)
{
    ui->setupUi(this);

    m_mapView = new QQuickWidget(this);

    // --- 1. CONFIGURATION AUTO-COMPLETION ---
    m_suggestModel = new QStringListModel(this);
    m_completer = new QCompleter(m_suggestModel, this);
    m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

    // MAQUILLAGE SOMBRE DU MENU (Réparé par l'include ci-dessus)
    QAbstractItemView* popup = m_completer->popup();
    if (popup) {
        popup->setStyleSheet(
            "QAbstractItemView { "
            "   background: #2a2f3a; "
            "   color: white; "
            "   border: 1px solid #3d4455; "
            "   selection-background-color: #4aa5ff; "
            "}"
            );
    }

    ui->editSearch->setCompleter(m_completer);

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

    connect(ui->editSearch, &QLineEdit::textChanged, m_searchTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    // --- 2. CONFIGURATION MAPBOX ---
    QString apiKey = QString::fromLocal8Bit(qgetenv("MAPBOX_KEY"));
    m_mapView->rootContext()->setContextProperty("mapboxApiKey", apiKey);

    m_mapView->setSource(QUrl("qrc:/map.qml"));
    m_mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    connect(m_mapView, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status){
        if (status == QQuickWidget::Ready) {
            connect(m_mapView->rootObject(), SIGNAL(routeInfoUpdated(QString,QString)),
                    this, SLOT(onRouteInfoReceived(QString,QString)));
            connect(m_mapView->rootObject(), SIGNAL(suggestionsReady(QStringList)),
                    this, SLOT(updateSuggestions(QStringList)));
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
            QMetaObject::invokeMethod(m_mapView->rootObject(), "searchDestination",
                                      Q_ARG(QVariant, destination));
        }
    });
}

NavigationPage::~NavigationPage(){ delete ui; }

void NavigationPage::updateSuggestions(const QStringList& suggestions) {
    m_suggestModel->setStringList(suggestions);
    m_completer->complete();
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
