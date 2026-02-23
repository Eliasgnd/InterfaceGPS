// Rôle architectural: implémentation de la gestion des réglages et du Bluetooth local.
// Responsabilités: scanner les périphériques, appliquer les règles d'exclusivité et exposer les retours d'état à l'utilisateur.
// Dépendances principales: bluetoothctl via QProcess, timers de surveillance et UI Qt Widgets.

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


    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(120000);
    connect(m_discoveryTimer, &QTimer::timeout, this, &SettingsPage::stopDiscovery);

    connect(m_localDevice, &QBluetoothLocalDevice::errorOccurred,
            this, &SettingsPage::errorOccurred);

    connect(ui->btnVisible, &QPushButton::clicked, this, &SettingsPage::onVisibleClicked);
    connect(ui->btnForget, &QPushButton::clicked, this, &SettingsPage::onForgetClicked);


    ui->btnForget->setEnabled(false);
    connect(ui->listDevices, &QListWidget::itemSelectionChanged, this, [this](){
        QListWidgetItem *item = ui->listDevices->currentItem();
        bool hasSelection = item && !item->data(Qt::UserRole).toString().isEmpty();
        ui->btnForget->setEnabled(hasSelection);
    });


    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &SettingsPage::refreshPairedList);
    m_pollTimer->start(2000);

    refreshPairedList();
}

SettingsPage::~SettingsPage() { delete ui; }

void SettingsPage::showAutoClosingMessage(const QString &title, const QString &text, int timeoutMs)
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(title);
    msgBox->setText(text);
    msgBox->setStandardButtons(QMessageBox::NoButton);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->show();


    QTimer::singleShot(timeoutMs, msgBox, [msgBox]() {
        msgBox->close();
        msgBox->deleteLater();
    });
}

void SettingsPage::refreshPairedList()
{
    // Source de vérité Bluetooth: bluetoothctl est interrogé périodiquement pour refléter l'état système réel.
    QProcess process;
    process.start("bluetoothctl", QStringList() << "devices");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();

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

    for (const QString &line : lines) {
        // Format attendu: "Device <MAC> <Nom>". Les lignes invalides sont ignorées.
        if (line.trimmed().isEmpty()) continue;

        QStringList parts = line.split(' ');
        if (parts.size() >= 3 && parts[0] == "Device") {
            QString mac = parts[1];
            QString name = parts.mid(2).join(' ');


            QProcess infoProcess;
            infoProcess.start("bluetoothctl", QStringList() << "info" << mac);
            infoProcess.waitForFinished();
            QString infoOutput = infoProcess.readAllStandardOutput();
            bool isConnected = infoOutput.contains("Connected: yes");

            if (isConnected) {
                connectedDevices.insert(mac, name);

                if (mac != m_lastActiveMac) {
                    newcomerMac = mac;
                    newcomerName = name;
                }
            }


            if (!m_knownMacs.contains(mac)) {
                m_knownMacs.insert(mac);
                QProcess::execute("bluetoothctl", QStringList() << "trust" << mac);
                if (ui->btnVisible->isChecked()) setDiscoverable(false);
            }


            QString label = name + " (" + mac + ")";
            if (isConnected) label += " (connecté)";

            QListWidgetItem *item = new QListWidgetItem("📱 " + label, ui->listDevices);
            item->setData(Qt::UserRole, mac);

            if (isConnected) {
                item->setForeground(Qt::green);
            }

            if (mac == selectedMac) {
                ui->listDevices->setCurrentItem(item);
            }
            found = true;
        }
    }


    // Règle métier: un seul appareil audio actif pour éviter les conflits A2DP/HFP.
    if (connectedDevices.size() > 1 && !newcomerMac.isEmpty()) {
        QString oldDeviceName;


        QMapIterator<QString, QString> i(connectedDevices);
        while (i.hasNext()) {
            i.next();
            if (i.key() != newcomerMac) {
                oldDeviceName = i.value();
                QProcess::execute("bluetoothctl", QStringList() << "disconnect" << i.key());
                qDebug() << "Exclusivité : Déconnexion de" << oldDeviceName;
            }
        }


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
    QString name = item->text().remove("📱 ").remove(" (connecté)");
    if (mac.isEmpty()) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Supprimer l'appareil",
                                  QString("Voulez-vous vraiment oublier l'appareil %1 ?").arg(name),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    QProcess::execute("bluetoothctl", QStringList() << "remove" << mac);

    m_knownMacs.remove(mac);
    if(m_lastActiveMac == mac) m_lastActiveMac = "";

    ui->listDevices->clear();
    ui->btnForget->setEnabled(false);

    refreshPairedList();
}

void SettingsPage::errorOccurred(QBluetoothLocalDevice::Error error)
{
    qDebug() << "Erreur Bluetooth LocalDevice:" << error;
}
