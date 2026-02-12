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

    // --- MODIFICATION : UTILISATION DE LA VARIABLE D'ENVIRONNEMENT ---
    // On récupère la variable, on la convertit et on nettoie les espaces éventuels
    QString apiKey = QString::fromLocal8Bit(qgetenv("MAPBOX_KEY")).trimmed();

    // Vérification de sécurité
    if (apiKey.isEmpty()) {
        qWarning() << "!!! ERREUR : La variable d'environnement MAPBOX_KEY est vide ou introuvable !!!";
        qWarning() << "Veuillez la configurer dans Qt Creator (Projets > Run > Environment) avec votre NOUVELLE clé.";
    } else {
        qDebug() << "OK : Clé API chargée depuis MAPBOX_KEY (" << apiKey.left(10) << "...)";
    }

    m_mapView->rootContext()->setContextProperty("mapboxApiKey", apiKey);
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
    };

    connect(m_mapView, &QQuickWidget::statusChanged, this, [this, setupQmlConnections](QQuickWidget::Status status){
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
