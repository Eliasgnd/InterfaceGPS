// Rôle architectural: implémentation de la page de navigation GPS.
// Responsabilités: piloter la carte QML, interroger les services de géocodage et synchroniser les suggestions de recherche.
// Dépendances principales: QQuickWidget/QQmlContext, QNetworkAccessManager et clavier virtuel.

#include "navigationpage.h"
#include "ui_navigationpage.h"
#include "telemetrydata.h"
#include "clavier.h"
#include <QCompleter>
#include <QStringListModel>
#include <QTimer>
#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariant>

NavigationPage::NavigationPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::NavigationPage)
{
    // Cette page joue le rôle de contrôleur: elle coordonne QML, réseau et clavier virtuel.
    ui->setupUi(this);


    m_suggestionsModel = new QStringListModel(this);
    m_searchCompleter = new QCompleter(m_suggestionsModel, this);
    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_searchCompleter->setFilterMode(Qt::MatchContains);
    m_searchCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    ui->editSearch->setCompleter(m_searchCompleter);



    ui->editSearch->installEventFilter(this);

    m_suggestionDebounceTimer = new QTimer(this);
    m_suggestionDebounceTimer->setSingleShot(true);
    m_suggestionDebounceTimer->setInterval(300);

    connect(m_searchCompleter, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated),
            this, &NavigationPage::onSuggestionChosen);

    connect(m_suggestionDebounceTimer, &QTimer::timeout,
            this, &NavigationPage::triggerSuggestionsSearch);

    connect(ui->editSearch, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (!m_ignoreTextUpdate && text.length() >= 2) {
            m_suggestionDebounceTimer->start();
        }
    });


    // QQuickWidget héberge la carte QML dans la hiérarchie QWidget existante.
    m_mapView = new QQuickWidget(this);
    QString mapboxKey = QString::fromLocal8Bit(qgetenv("MAPBOX_API_KEY")).trimmed();
    QString hereKey = QString::fromLocal8Bit(qgetenv("HERE_API_KEY")).trimmed();

    m_mapView->rootContext()->setContextProperty("mapboxApiKey", mapboxKey);
    m_mapView->rootContext()->setContextProperty("hereApiKey", hereKey);
    m_mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    auto setupQmlConnections = [this]() {
        QObject* root = m_mapView->rootObject();
        if (!root) return;
        connect(root, SIGNAL(routeInfoUpdated(QString,QString)), this, SLOT(onRouteInfoReceived(QString,QString)));
        connect(root, SIGNAL(suggestionsUpdated(QString)), this, SLOT(onSuggestionsReceived(QString)));
        connect(root, SIGNAL(routeReadyForSimulation(QVariant)), this, SLOT(onRouteReadyForSimulation(QVariant)));
    };

    connect(m_mapView, &QQuickWidget::statusChanged, this, [setupQmlConnections](QQuickWidget::Status status){
        if (status == QQuickWidget::Ready) setupQmlConnections();
    });

    m_mapView->setSource(QUrl("qrc:/map.qml"));
    ui->lblMap->setVisible(false);
    ui->mapLayout->addWidget(m_mapView);


    connect(ui->btnZoomIn, &QPushButton::clicked, this, [this](){
        if (m_mapView && m_mapView->rootObject()) {
            double z = m_mapView->rootObject()->property("carZoom").toDouble();
            m_mapView->rootObject()->setProperty("carZoom", z + 1);
        }
    });
    connect(ui->btnZoomOut, &QPushButton::clicked, this, [this](){
        if (m_mapView && m_mapView->rootObject()) {
            double z = m_mapView->rootObject()->property("carZoom").toDouble();
            m_mapView->rootObject()->setProperty("carZoom", z - 1);
        }
    });
    connect(ui->btnCenter, &QPushButton::clicked, this, [this](){
        if(m_mapView && m_mapView->rootObject()) QMetaObject::invokeMethod(m_mapView->rootObject(), "recenterMap");
    });
    connect(ui->btnSearch, &QPushButton::clicked, this, [this](){
        requestRouteForText(ui->editSearch->text());
    });
    connect(ui->btnSimulate, &QPushButton::clicked, this, [this](){
        if(m_t && !m_lastCalculatedRoute.isEmpty()){
            emit m_t->simulateRouteRequested(m_lastCalculatedRoute);
        }
    });
}

bool NavigationPage::eventFilter(QObject *obj, QEvent *event)
{
    // On intercepte le focus de la barre de recherche pour imposer le clavier applicatif.
    if (obj == ui->editSearch && event->type() == QEvent::MouseButtonPress) {
        openVirtualKeyboard();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void NavigationPage::openVirtualKeyboard()
{
    // Le clavier est modal pour éviter la coexistence avec le clavier système Qt.
    Clavier clavier(this);
    m_currentClavier = &clavier;


    clavier.setInitialText(ui->editSearch->text());




    connect(&clavier, &Clavier::textChangedExternally, this, [this](const QString &text){
        ui->editSearch->setText(text);

        triggerSuggestionsSearch();
    });

    if (clavier.exec() == QDialog::Accepted) {
        QString res = clavier.getText();
        ui->editSearch->setText(res);
        requestRouteForText(res);
    }

    m_currentClavier = nullptr;
}

NavigationPage::~NavigationPage(){ delete ui; }

void NavigationPage::onSuggestionsReceived(const QString& jsonSuggestions) {
    // Deux sorties sont alimentées: la liste Qt Widgets et la liste du clavier custom.
    QJsonDocument doc = QJsonDocument::fromJson(jsonSuggestions.toUtf8());
    QJsonArray arr = doc.array();
    QStringList suggestions;
    for(const auto& val : arr) suggestions << val.toString();


    m_suggestionsModel->setStringList(suggestions);


    if (m_currentClavier) {
        m_currentClavier->displaySuggestions(suggestions);
    }
}

void NavigationPage::onRouteInfoReceived(const QString& distance, const QString& duration) {
    ui->lblLat->setText("Dist: " + distance);
    ui->lblLon->setText("Temps: " + duration);
}

void NavigationPage::bindTelemetry(TelemetryData* t) {
    m_t = t;
    if(!m_t) return;
    auto refresh = [this](){
        if(m_mapView && m_mapView->rootObject()){
            m_mapView->rootObject()->setProperty("carLat", m_t->lat());
            m_mapView->rootObject()->setProperty("carLon", m_t->lon());
            m_mapView->rootObject()->setProperty("carHeading", m_t->heading());
            m_mapView->rootObject()->setProperty("carSpeed", m_t->speedKmh());
        }
    };
    connect(m_t, &TelemetryData::latChanged, this, refresh);
    connect(m_t, &TelemetryData::lonChanged, this, refresh);
    connect(m_t, &TelemetryData::headingChanged, this, refresh);
    connect(m_t, &TelemetryData::speedKmhChanged, this, refresh);
    refresh();
}

void NavigationPage::requestRouteForText(const QString& destination) {
    if (destination.trimmed().isEmpty() || !m_mapView || !m_mapView->rootObject()) return;
    QMetaObject::invokeMethod(m_mapView->rootObject(), "searchDestination",
                              Q_ARG(QVariant, destination.trimmed()));
}

void NavigationPage::onSuggestionChosen(const QString& suggestion) {
    m_ignoreTextUpdate = true;
    ui->editSearch->setText(suggestion);
    m_ignoreTextUpdate = false;
    requestRouteForText(suggestion);
}

void NavigationPage::triggerSuggestionsSearch() {
    // Le debouncing côté QTimer limite les appels Mapbox pendant la frappe rapide.
    QString query = ui->editSearch->text().trimmed();
    if (query.size() < 3) return;
    if (m_mapView && m_mapView->rootObject()) {
        QMetaObject::invokeMethod(m_mapView->rootObject(), "requestSuggestions", Q_ARG(QVariant, query));
    }
}

void NavigationPage::onRouteReadyForSimulation(const QVariant& pathObj) {
    m_lastCalculatedRoute = pathObj.toList();
}
