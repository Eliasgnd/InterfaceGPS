/**
 * @file settingspage.cpp
 * @brief Implémentation de la gestion des réglages et du Bluetooth local.
 * @details Responsabilités : Scanner les périphériques, appliquer les règles d'exclusivité
 * (un seul téléphone connecté à la fois) et exposer les retours d'état à l'utilisateur.
 * Dépendances principales : Utilitaire Linux 'bluetoothctl' appelé via QProcess,
 * timers de surveillance (Polling) et UI Qt Widgets.
 */

#include "settingspage.h"
#include "ui_settingspage.h"
#include "telemetrydata.h"
#include <QMessageBox>
#include <QDebug>
#include <QProcess>
#include <QTimer>

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::SettingsPage)
{
    ui->setupUi(this);

    m_lastActiveMac = "";
    m_localDevice = new QBluetoothLocalDevice(this);

    // --- GESTION DE LA DÉCOUVRABILITÉ (SÉCURITÉ) ---
    // Le mode appairage est désactivé automatiquement après 2 minutes (120 000 ms)
    // pour éviter que le véhicule ne reste vulnérable en permanence.
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(120000);
    connect(m_discoveryTimer, &QTimer::timeout, this, &SettingsPage::stopDiscovery);

    connect(m_localDevice, &QBluetoothLocalDevice::errorOccurred,
            this, &SettingsPage::errorOccurred);

    // Connexion des boutons de l'interface
    connect(ui->btnVisible, &QPushButton::clicked, this, &SettingsPage::onVisibleClicked);
    connect(ui->btnForget, &QPushButton::clicked, this, &SettingsPage::onForgetClicked);

    // Le bouton "Oublier" ne s'active que si un appareil est explicitement sélectionné dans la liste
    ui->btnForget->setEnabled(false);
    connect(ui->listDevices, &QListWidget::itemSelectionChanged, this, [this](){
        QListWidgetItem *item = ui->listDevices->currentItem();
        bool hasSelection = item && !item->data(Qt::UserRole).toString().isEmpty();
        ui->btnForget->setEnabled(hasSelection);
    });

    // --- SYNCHRONISATION EN TEMPS RÉEL (POLLING) ---
    // Interroge le système d'exploitation toutes les 2 secondes pour voir les nouveaux appareils
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &SettingsPage::refreshPairedList);
    m_pollTimer->start(2000);

    refreshPairedList();
}

SettingsPage::~SettingsPage() {
    delete ui;
}

void SettingsPage::showAutoClosingMessage(const QString &title, const QString &text, int timeoutMs)
{
    // Crée une boîte de dialogue (Popup) non bloquante
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(title);
    msgBox->setText(text);
    msgBox->setStandardButtons(QMessageBox::NoButton); // Pas de bouton "OK" à cliquer
    msgBox->setIcon(QMessageBox::Information);
    msgBox->show();

    // Fermeture propre et libération mémoire après le délai demandé
    QTimer::singleShot(timeoutMs, msgBox, [msgBox]() {
        msgBox->close();
        msgBox->deleteLater();
    });
}

void SettingsPage::refreshPairedList()
{
    // Source de vérité Bluetooth : bluetoothctl est interrogé périodiquement via des commandes shell
    // pour refléter l'état système réel sous Linux (plus fiable que QBluetoothDeviceDiscoveryAgent dans ce contexte).
    QProcess process;
    process.start("bluetoothctl", QStringList() << "devices");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();

    // Mémorisation de la sélection utilisateur pour ne pas la perdre lors du rafraîchissement
    QString selectedMac;
    if (ui->listDevices->currentItem()) {
        selectedMac = ui->listDevices->currentItem()->data(Qt::UserRole).toString();
    }

    ui->listDevices->clear();

    QStringList lines = output.split('\n');
    bool found = false;

    QString newcomerMac;
    QString newcomerName;
    QMap<QString, QString> connectedDevices;

    // Analyse (Parsing) de la sortie de bluetoothctl
    for (const QString &line : lines) {
        // Format attendu: "Device <MAC> <Nom>". Les lignes invalides sont ignorées.
        if (line.trimmed().isEmpty()) continue;

        QStringList parts = line.split(' ');
        if (parts.size() >= 3 && parts[0] == "Device") {
            QString mac = parts[1];
            QString name = parts.mid(2).join(' '); // Le nom peut contenir des espaces

            // On lance une seconde requête pour savoir si cet appareil spécifique est actuellement connecté
            QProcess infoProcess;
            infoProcess.start("bluetoothctl", QStringList() << "info" << mac);
            infoProcess.waitForFinished();
            QString infoOutput = infoProcess.readAllStandardOutput();
            bool isConnected = infoOutput.contains("Connected: yes");

            if (isConnected) {
                connectedDevices.insert(mac, name);

                // Détection d'un nouvel appareil venant de se connecter
                if (mac != m_lastActiveMac) {
                    newcomerMac = mac;
                    newcomerName = name;
                }
            }

            // Trust automatique : Dès qu'un appareil est vu, on lui fait confiance au niveau système
            // pour faciliter les reconnexions futures sans demande PIN.
            if (!m_knownMacs.contains(mac)) {
                m_knownMacs.insert(mac);
                QProcess::execute("bluetoothctl", QStringList() << "trust" << mac);

                // Si on était en mode "Visible" et qu'un appareil s'est appairé, on cache le véhicule
                if (ui->btnVisible->isChecked()) setDiscoverable(false);
            }

            // Construction de l'élément visuel pour la liste
            QString label = name + " (" + mac + ")";
            if (isConnected) label += " (connecté)";

            QListWidgetItem *item = new QListWidgetItem("📱 " + label, ui->listDevices);
            item->setData(Qt::UserRole, mac); // Stockage de l'adresse MAC de manière invisible

            // Mise en valeur visuelle (Vert) si l'appareil est connecté
            if (isConnected) {
                item->setForeground(Qt::green);
            }

            // Restauration de la sélection
            if (mac == selectedMac) {
                ui->listDevices->setCurrentItem(item);
            }
            found = true;
        }
    }

    // --- RÈGLE MÉTIER : EXCLUSIVITÉ BLUETOOTH ---
    // Un seul appareil audio doit être actif à la fois pour éviter les conflits de flux A2DP/HFP
    // et les plantages du démon Bluetooth (BlueZ).
    if (connectedDevices.size() > 1 && !newcomerMac.isEmpty()) {
        QString oldDeviceName;

        QMapIterator<QString, QString> i(connectedDevices);
        while (i.hasNext()) {
            i.next();
            // On déconnecte de force tous les autres appareils sauf le petit nouveau
            if (i.key() != newcomerMac) {
                oldDeviceName = i.value();
                QProcess::execute("bluetoothctl", QStringList() << "disconnect" << i.key());
                qDebug() << "Exclusivité : Déconnexion de" << oldDeviceName;
            }
        }

        // Information de l'utilisateur sur cette déconnexion automatique
        showAutoClosingMessage("Changement d'appareil",
                               QString("Priorité à '%1'.\n'%2' a été déconnecté.")
                                   .arg(newcomerName).arg(oldDeviceName),
                               3000);

        m_lastActiveMac = newcomerMac;
    }
    else if (connectedDevices.size() == 1) {
        m_lastActiveMac = connectedDevices.keys().first();
    }
    else if (connectedDevices.isEmpty()) {
        m_lastActiveMac = "";
    }

    // Affichage par défaut si la liste est vide
    if (!found) {
        new QListWidgetItem("(Aucun appareil enregistré)", ui->listDevices);
        ui->btnForget->setEnabled(false);
    }
}

void SettingsPage::onVisibleClicked()
{
    setDiscoverable(ui->btnVisible->isChecked());
}

void SettingsPage::setDiscoverable(bool enable)
{
    if (enable) {
        QProcess::execute("bluetoothctl", QStringList() << "discoverable" << "on");
        ui->btnVisible->setText("Visible (120s max)...");
        ui->btnVisible->setChecked(true);

        // Fenêtre courte pour limiter l'exposition Bluetooth permanente du véhicule.
        m_discoveryTimer->start();
    } else {
        QProcess::execute("bluetoothctl", QStringList() << "discoverable" << "off");
        ui->btnVisible->setText("Rendre Visible (Appairage)");
        ui->btnVisible->setChecked(false);
        m_discoveryTimer->stop();
    }
}

void SettingsPage::stopDiscovery()
{
    setDiscoverable(false);
}

void SettingsPage::onForgetClicked()
{
    QListWidgetItem *item = ui->listDevices->currentItem();
    if (!item) return;

    QString mac = item->data(Qt::UserRole).toString();
    QString name = item->text().remove("📱 ").remove(" (connecté)"); // Nettoyage du texte affiché
    if (mac.isEmpty()) return;

    // Demande de confirmation à l'utilisateur
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Supprimer l'appareil",
                                  QString("Voulez-vous vraiment oublier l'appareil %1 ?").arg(name),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    // Suppression définitive au niveau du système d'exploitation
    QProcess::execute("bluetoothctl", QStringList() << "remove" << mac);

    m_knownMacs.remove(mac);
    if(m_lastActiveMac == mac) m_lastActiveMac = "";

    // Vidage manuel de la liste pour forcer un rafraîchissement visuel immédiat
    ui->listDevices->clear();
    ui->btnForget->setEnabled(false);

    refreshPairedList();
}

void SettingsPage::errorOccurred(QBluetoothLocalDevice::Error error)
{
    qDebug() << "Erreur Bluetooth LocalDevice:" << error;
}
