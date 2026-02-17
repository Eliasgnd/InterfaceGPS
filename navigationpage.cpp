#include "navigationpage.h"
#include "ui_navigationpage.h"
#include "telemetrydata.h"
#include "clavier.h" // <--- INCLUSION DE TON CLAVIER
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

    // --- 2. INSTALLATION DE L'EVENT FILTER ---
    // On dit à editSearch de nous envoyer ses événements (clics, etc.)
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

    // --- 3. CONFIGURATION CARTE QML ---
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

    // --- 4. BOUTONS ---
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

// --- GESTION DU CLIC SUR LA BARRE DE RECHERCHE ---
bool NavigationPage::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->editSearch && event->type() == QEvent::MouseButtonPress) {
        openVirtualKeyboard();
        return true; // On arrête l'événement ici pour ne pas avoir le focus standard
    }
    return QWidget::eventFilter(obj, event);
}

void NavigationPage::openVirtualKeyboard()
{
    Clavier clavier(this);
    m_currentClavier = &clavier; // On enregistre le pointeur

    // On pré-remplit le clavier avec ce qui est déjà écrit
    clavier.setInitialText(ui->editSearch->text());

    // --- CONNEXION TEMPS RÉEL ---
    // Dès qu'on tape sur le clavier, on met à jour la barre de recherche invisible
    // et on déclenche la recherche Mapbox
    connect(&clavier, &Clavier::textChangedExternally, this, [this](const QString &text){
        ui->editSearch->setText(text);
        // On force le déclenchement de la recherche de suggestions
        triggerSuggestionsSearch();
    });

    if (clavier.exec() == QDialog::Accepted) {
        QString res = clavier.getText();
        ui->editSearch->setText(res);
        requestRouteForText(res);
    }

    m_currentClavier = nullptr; // On nettoie le pointeur après fermeture
}

NavigationPage::~NavigationPage(){ delete ui; }

void NavigationPage::onSuggestionsReceived(const QString& jsonSuggestions) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonSuggestions.toUtf8());
    QJsonArray arr = doc.array();
    QStringList suggestions;
    for(const auto& val : arr) suggestions << val.toString();

    // 1. Mise à jour de la liste standard (si le clavier est fermé)
    m_suggestionsModel->setStringList(suggestions);

    // 2. NOUVEAU : Si le clavier est ouvert, on lui envoie les suggestions !
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
    QString query = ui->editSearch->text().trimmed();
    if (query.size() < 3) return;
    if (m_mapView && m_mapView->rootObject()) {
        QMetaObject::invokeMethod(m_mapView->rootObject(), "requestSuggestions", Q_ARG(QVariant, query));
    }
}

void NavigationPage::onRouteReadyForSimulation(const QVariant& pathObj) {
    m_lastCalculatedRoute = pathObj.toList();
}
