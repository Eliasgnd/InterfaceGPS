/**
 * @file navigationpage.cpp
 * @brief Implémentation de la page de navigation GPS.
 * @details Pilote la carte QML, interroge les services de géocodage,
 * gère le système d'autocomplétion (avec timer anti-rebond) et instancie le clavier tactile.
 */

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
    // Cette page joue le rôle de contrôleur : elle coordonne QML, requêtes réseau et clavier virtuel.
    ui->setupUi(this);

    // --- CONFIGURATION DE L'AUTOCOMPLÉTION ---
    m_suggestionsModel = new QStringListModel(this);
    m_searchCompleter = new QCompleter(m_suggestionsModel, this);
    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_searchCompleter->setFilterMode(Qt::MatchContains);
    m_searchCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    ui->editSearch->setCompleter(m_searchCompleter);

    // Installation du filtre d'événement pour intercepter le clic sur la barre de recherche
    ui->editSearch->installEventFilter(this);

    // Timer de Debounce : attend 300ms après la dernière touche frappée avant de lancer la recherche.
    // Cela évite d'épuiser le quota de requêtes API Mapbox/Here quand on tape vite.
    m_suggestionDebounceTimer = new QTimer(this);
    m_suggestionDebounceTimer->setSingleShot(true);
    m_suggestionDebounceTimer->setInterval(300);

    // Connexions pour l'autocomplétion
    connect(m_searchCompleter, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated),
            this, &NavigationPage::onSuggestionChosen);

    connect(m_suggestionDebounceTimer, &QTimer::timeout,
            this, &NavigationPage::triggerSuggestionsSearch);

    connect(ui->editSearch, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (!m_ignoreTextUpdate && text.length() >= 2) {
            m_suggestionDebounceTimer->start(); // Relance le timer à chaque nouvelle lettre
        }
    });

    // --- CONFIGURATION DE LA CARTE QML ---
    // QQuickWidget héberge la carte QML dans la hiérarchie QWidget existante.
    m_mapView = new QQuickWidget(this);

    // Récupération des clés API depuis les variables d'environnement
    QString mapboxKey = QString::fromLocal8Bit(qgetenv("MAPBOX_API_KEY")).trimmed();

    // Injection des clés API dans le contexte QML pour qu'elles soient lisibles par la carte
    m_mapView->rootContext()->setContextProperty("mapboxApiKey", mapboxKey);
    m_mapView->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Fonction lambda pour lier les signaux QML aux slots C++ une fois la carte chargée
    auto setupQmlConnections = [this]() {
        QObject* root = m_mapView->rootObject();
        if (!root) return;
        connect(root, SIGNAL(routeInfoUpdated(QString,QString)), this, SLOT(onRouteInfoReceived(QString,QString)));
        connect(root, SIGNAL(suggestionsUpdated(QString)), this, SLOT(onSuggestionsReceived(QString)));
    };

    // Attendre que le QQuickWidget soit prêt avant de faire les connexions QML
    connect(m_mapView, &QQuickWidget::statusChanged, this, [setupQmlConnections](QQuickWidget::Status status){
        if (status == QQuickWidget::Ready) setupQmlConnections();
    });

    // Chargement du fichier QML et ajout au layout de l'interface
    m_mapView->setSource(QUrl("qrc:/map.qml"));
    ui->mapLayout->addWidget(m_mapView);

    // --- CONNEXION DES BOUTONS DE CONTRÔLE DE LA CARTE ---
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
        if(m_mapView && m_mapView->rootObject()) {
            QMetaObject::invokeMethod(m_mapView->rootObject(), "recenterMap");
        }
    });
    connect(ui->btnSearch, &QPushButton::clicked, this, [this](){
        requestRouteForText(ui->editSearch->text());
    });
}

NavigationPage::~NavigationPage() {
    delete ui;
}

bool NavigationPage::eventFilter(QObject *obj, QEvent *event)
{
    // On intercepte le focus de la barre de recherche (clic de souris)
    // pour imposer le clavier tactile applicatif au lieu du clavier système Windows/Linux.
    if (obj == ui->editSearch && event->type() == QEvent::MouseButtonPress) {
        openVirtualKeyboard();
        return true; // L'événement est consommé
    }
    return QWidget::eventFilter(obj, event);
}

void NavigationPage::openVirtualKeyboard()
{
    // Le clavier est modal (bloquant) pour éviter la coexistence avec un clavier système Qt.
    Clavier clavier(this);
    m_currentClavier = &clavier;

    clavier.setInitialText(ui->editSearch->text());

    // Permet de mettre à jour la barre de recherche en temps réel pendant la frappe sur le clavier custom
    connect(&clavier, &Clavier::textChangedExternally, this, [this](const QString &text){
        ui->editSearch->setText(text);
        triggerSuggestionsSearch(); // Relance la recherche de suggestions dynamiques
    });

    // Si l'utilisateur valide sa saisie (Touche Entrée sur le clavier)
    if (clavier.exec() == QDialog::Accepted) {
        QString res = clavier.getText();
        ui->editSearch->setText(res);
        requestRouteForText(res);
    }

    m_currentClavier = nullptr;
}

void NavigationPage::onSuggestionsReceived(const QString& jsonSuggestions) {
    // Les suggestions renvoyées par QML (Mapbox) arrivent au format JSON.
    // On les décode pour alimenter :
    // 1. La liste du Completer Qt (si on tape au clavier physique)
    // 2. La liste du clavier tactile custom

    QJsonDocument doc = QJsonDocument::fromJson(jsonSuggestions.toUtf8());
    QJsonArray arr = doc.array();
    QStringList suggestions;

    for(const auto& val : arr) {
        suggestions << val.toString();
    }

    m_suggestionsModel->setStringList(suggestions);

    // Transmission des suggestions au clavier virtuel s'il est ouvert
    if (m_currentClavier) {
        m_currentClavier->displaySuggestions(suggestions);
    }
}

void NavigationPage::onRouteInfoReceived(const QString& distance, const QString& duration) {
    // Note : On utilise les labels historiquement nommés "lblLat" et "lblLon"
    // pour afficher la distance et le temps restant du trajet.
    ui->lblLat->setText("Dist: " + distance);
    ui->lblLon->setText("Temps: " + duration);
}

void NavigationPage::bindTelemetry(TelemetryData* t) {
    m_t = t;
    if(!m_t) return;

    // Fonction lambda pour synchroniser les données GPS C++ vers les propriétés de la carte QML
    auto refresh = [this](){
        if(m_mapView && m_mapView->rootObject()){
            m_mapView->rootObject()->setProperty("carLat", m_t->lat());
            m_mapView->rootObject()->setProperty("carLon", m_t->lon());
            m_mapView->rootObject()->setProperty("carHeading", m_t->heading());
            m_mapView->rootObject()->setProperty("carSpeed", m_t->speedKmh());
        }
    };

    // Connexion aux signaux de télémétrie pour une mise à jour en temps réel
    connect(m_t, &TelemetryData::latChanged, this, refresh);
    connect(m_t, &TelemetryData::lonChanged, this, refresh);
    connect(m_t, &TelemetryData::headingChanged, this, refresh);
    connect(m_t, &TelemetryData::speedKmhChanged, this, refresh);

    refresh(); // Premier appel pour initialiser la carte avec les valeurs actuelles
}

void NavigationPage::requestRouteForText(const QString& destination) {
    if (destination.trimmed().isEmpty() || !m_mapView || !m_mapView->rootObject()) return;

    // Appel d'une fonction Javascript/QML directement depuis le C++
    QMetaObject::invokeMethod(m_mapView->rootObject(), "searchDestination",
                              Q_ARG(QVariant, destination.trimmed()));
}

void NavigationPage::onSuggestionChosen(const QString& suggestion) {
    // Bloque temporairement l'événement de modification de texte pour éviter
    // que la sélection d'une suggestion ne relance une nouvelle recherche de suggestions (boucle infinie).
    m_ignoreTextUpdate = true;
    ui->editSearch->setText(suggestion);
    m_ignoreTextUpdate = false;

    requestRouteForText(suggestion);
}

void NavigationPage::triggerSuggestionsSearch() {
    QString query = ui->editSearch->text().trimmed();

    // Inutile de chercher pour moins de 3 caractères
    if (query.size() < 3) return;

    if (m_mapView && m_mapView->rootObject()) {
        // Envoie la requête au script QML pour interrogation de l'API cartographique
        QMetaObject::invokeMethod(m_mapView->rootObject(), "requestSuggestions", Q_ARG(QVariant, query));
    }
}
