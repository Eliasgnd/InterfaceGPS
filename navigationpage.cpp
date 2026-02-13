#include "navigationpage.h"
#include "ui_navigationpage.h"
#include "telemetrydata.h"
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
    ui->setupUi(this);

    // --- 1. CONFIGURATION AUTOCOMPLETION ---
    m_suggestionsModel = new QStringListModel(this);
    m_searchCompleter = new QCompleter(m_suggestionsModel, this);
    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_searchCompleter->setFilterMode(Qt::MatchContains);
    m_searchCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    ui->editSearch->setCompleter(m_searchCompleter);

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

    // --- 2. CONFIGURATION CARTE QML ---
    m_mapView = new QQuickWidget(this);

    // --- MODIFICATION : LECTURE DES VARIABLES D'ENVIRONNEMENT ---
    // On lit MAPBOX_API_KEY et HERE_API_KEY définies dans Qt Creator (Projets > Run)
    QString mapboxKey = QString::fromLocal8Bit(qgetenv("MAPBOX_API_KEY")).trimmed();
    QString hereKey = QString::fromLocal8Bit(qgetenv("HERE_API_KEY")).trimmed();

    // Logs pour vérifier (affichera les 10 premiers caractères)
    if (mapboxKey.isEmpty()) qWarning() << "⚠️ MAPBOX_API_KEY est vide !";
    else qDebug() << "✅ Mapbox Key chargée :" << mapboxKey.left(10) << "...";

    if (hereKey.isEmpty()) qWarning() << "⚠️ HERE_API_KEY est vide !";
    else qDebug() << "✅ HERE Key chargée :" << hereKey.left(10) << "...";

    // INJECTION DANS QML : On rend ces variables accessibles sous ces noms exacts
    m_mapView->rootContext()->setContextProperty("mapboxApiKey", mapboxKey);
    m_mapView->rootContext()->setContextProperty("hereApiKey", hereKey);

    m_mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Connexion des signaux
    auto setupQmlConnections = [this]() {
        QObject* root = m_mapView->rootObject();
        if (!root) return;
        disconnect(root, nullptr, this, nullptr);

        connect(root, SIGNAL(routeInfoUpdated(QString,QString)),
                this, SLOT(onRouteInfoReceived(QString,QString)));

        connect(root, SIGNAL(suggestionsUpdated(QString)),
                this, SLOT(onSuggestionsReceived(QString)));

        // NOUVELLE LIGNE :
        connect(root, SIGNAL(routeReadyForSimulation(QVariant)),
                this, SLOT(onRouteReadyForSimulation(QVariant)));
    };

    connect(m_mapView, &QQuickWidget::statusChanged, this, [setupQmlConnections](QQuickWidget::Status status){
        if (status == QQuickWidget::Ready) {
            setupQmlConnections();
        }
    });

    m_mapView->setSource(QUrl("qrc:/map.qml"));

    if (m_mapView->status() == QQuickWidget::Ready) {
        setupQmlConnections();
    }

    ui->lblMap->setVisible(false);
    ui->mapLayout->addWidget(m_mapView);

    // --- 3. BOUTONS ---
    connect(ui->btnZoomIn, &QPushButton::clicked, this, [this](){
        if (!m_mapView || !m_mapView->rootObject()) return;
        double z = m_mapView->rootObject()->property("carZoom").toDouble();
        m_mapView->rootObject()->setProperty("carZoom", z + 1);
    });
    connect(ui->btnZoomOut, &QPushButton::clicked, this, [this](){
        if (!m_mapView || !m_mapView->rootObject()) return;
        double z = m_mapView->rootObject()->property("carZoom").toDouble();
        m_mapView->rootObject()->setProperty("carZoom", z - 1);
    });
    connect(ui->btnCenter, &QPushButton::clicked, this, [this](){
        if(m_mapView && m_mapView->rootObject()) QMetaObject::invokeMethod(m_mapView->rootObject(), "recenterMap");
    });
    connect(ui->btnSearch, &QPushButton::clicked, this, [this](){
        requestRouteForText(ui->editSearch->text());
    });
    connect(ui->editSearch, &QLineEdit::returnPressed, this, [this](){
        requestRouteForText(ui->editSearch->text());
        ui->editSearch->clearFocus();
    });


    // 3. Toujours dans le constructeur, avec les connexions des autres boutons (vers btnCenter, btnZoomIn...), reliez le nouveau bouton de simulation :
    connect(ui->btnSimulate, &QPushButton::clicked, this, [this](){
        if(m_t && !m_lastCalculatedRoute.isEmpty()){
            emit m_t->simulateRouteRequested(m_lastCalculatedRoute);
        }
    });
}

NavigationPage::~NavigationPage(){ delete ui; }

void NavigationPage::onSuggestionsReceived(const QString& jsonSuggestions) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonSuggestions.toUtf8());
    QJsonArray arr = doc.array();
    QStringList suggestions;
    for(const auto& val : arr) {
        suggestions << val.toString();
    }
    m_suggestionsModel->setStringList(suggestions);
    if (!suggestions.isEmpty() && ui->editSearch->hasFocus()) {
        m_searchCompleter->complete();
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
    refresh();
    connect(m_t, &TelemetryData::latChanged, this, refresh);
    connect(m_t, &TelemetryData::lonChanged, this, refresh);
    connect(m_t, &TelemetryData::headingChanged, this, refresh);
    connect(m_t, &TelemetryData::speedKmhChanged, this, refresh);
}

void NavigationPage::requestRouteForText(const QString& destination)
{
    if (destination.trimmed().isEmpty() || !m_mapView || !m_mapView->rootObject()) return;
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
    QString query = ui->editSearch->text().trimmed();
    if (query.size() < 3) {
        if (m_suggestionsModel) m_suggestionsModel->setStringList({});
        return;
    }
    if (m_mapView && m_mapView->rootObject()) {
        QMetaObject::invokeMethod(m_mapView->rootObject(), "requestSuggestions",
                                  Q_ARG(QVariant, query));
    }
}

void NavigationPage::onRouteReadyForSimulation(const QVariant& pathObj) {
    m_lastCalculatedRoute = pathObj.toList();
}
