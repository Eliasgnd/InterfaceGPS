#include "navigationpage.h"
#include "qdir.h"
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

    qDebug() << "=== INIT: NavigationPage démarrée ===";

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

    QString apiKey = QString::fromLocal8Bit(qgetenv("MAPBOX_KEY"));
    if(apiKey.isEmpty()) qDebug() << "!!! ATTENTION: MAPBOX_KEY est vide !!!";
    else qDebug() << "OK: Clé API trouvée (" << apiKey.left(5) << "...)";

    m_mapView->rootContext()->setContextProperty("mapboxApiKey", apiKey);
    m_mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // --- CORRECTION CRITIQUE ICI ---

    // On définit la logique de connexion dans une lambda pour pouvoir l'appeler à deux endroits
    auto setupQmlConnections = [this]() {
        QObject* root = m_mapView->rootObject();
        if (!root) return;

        // Pour éviter de connecter 2 fois si on appelle cette fonction plusieurs fois
        disconnect(root, nullptr, this, nullptr);

        qDebug() << ">>> SETUP: Connexion des signaux QML -> C++...";

        // Connexion Route
        connect(root, SIGNAL(routeInfoUpdated(QString,QString)),
                this, SLOT(onRouteInfoReceived(QString,QString)));

        // Connexion Suggestions
        bool success = connect(root, SIGNAL(suggestionsUpdated(QString)),
                               this, SLOT(onSuggestionsReceived(QString)));

        if (success) {
            qDebug() << ">>> SUCCÈS: Signal suggestionsUpdated connecté !";
        } else {
            qDebug() << "!!! ÉCHEC: Impossible de connecter suggestionsUpdated !!!";
        }
    };

    // 1. On écoute le changement de statut (AVANT de charger la source)
    connect(m_mapView, &QQuickWidget::statusChanged, this, [this, setupQmlConnections](QQuickWidget::Status status){
        if (status == QQuickWidget::Ready) {
            qDebug() << "EVENT: QML passé à l'état Ready via signal.";
            setupQmlConnections();
        } else if (status == QQuickWidget::Error) {
            qDebug() << "!!! ERREUR QML:" << m_mapView->errors();
        }
    });

    // 2. On charge la source (C'est ça qui déclenche le chargement)
    m_mapView->setSource(QUrl("qrc:/map.qml"));

    // 3. Sécurité : Si le chargement était instantané (déjà Ready), on connecte manuellement tout de suite
    if (m_mapView->status() == QQuickWidget::Ready) {
        qDebug() << "EVENT: QML était déjà Ready immédiatement.";
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
    qDebug() << "\n>>> [C++] RECEPTION DU JSON QML:" << jsonSuggestions;

    QJsonDocument doc = QJsonDocument::fromJson(jsonSuggestions.toUtf8());
    QJsonArray arr = doc.array();

    QStringList suggestions;
    for(const auto& val : arr) {
        suggestions << val.toString();
    }

    m_suggestionsModel->setStringList(suggestions);

    if (!suggestions.isEmpty() && ui->editSearch->hasFocus()) {
        qDebug() << ">>> [C++] Ouverture du Popup";
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


