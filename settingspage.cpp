#include "settingspage.h"
#include "ui_settingspage.h"
#include "telemetrydata.h"
#include <QMessageBox>
#include <QDebug>
#include <QProcess>

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::SettingsPage)
{
    ui->setupUi(this);

    // Connexion du slider de luminosité
    connect(ui->sliderBrightness, &QSlider::valueChanged, this, [this](int v){
        ui->lblBrightnessValue->setText(QString::number(v));
        emit brightnessChanged(v);
    });

    // Connexion du changement de thème
    connect(ui->cmbTheme, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx){
                emit themeChanged(idx);
            });

    m_localDevice = new QBluetoothLocalDevice(this);

    // Timer pour la visibilité Bluetooth (120 secondes)
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(120000);
    connect(m_discoveryTimer, &QTimer::timeout, this, &SettingsPage::stopDiscovery);

    connect(m_localDevice, &QBluetoothLocalDevice::errorOccurred,
            this, &SettingsPage::errorOccurred);

    connect(ui->btnVisible, &QPushButton::clicked, this, &SettingsPage::onVisibleClicked);
    connect(ui->btnForget, &QPushButton::clicked, this, &SettingsPage::onForgetClicked);

    // --- LOGIQUE D'ACTIVATION DU BOUTON SUPPRIMER ---
    // Le bouton est désactivé par défaut
    ui->btnForget->setEnabled(false);

    // Il ne s'active que si un appareil valide est sélectionné dans la liste
    connect(ui->listDevices, &QListWidget::itemSelectionChanged, this, [this](){
        QListWidgetItem *item = ui->listDevices->currentItem();
        // On vérifie que l'item existe et possède une adresse MAC (Data Role)
        bool hasSelection = item && !item->data(Qt::UserRole).toString().isEmpty();
        ui->btnForget->setEnabled(hasSelection);
    });

    // --- TIMER DE SURVEILLANCE ---
    // Vérifie l'état des appareils toutes les 2 secondes
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &SettingsPage::refreshPairedList);
    m_pollTimer->start(2000);

    refreshPairedList();
}

SettingsPage::~SettingsPage(){ delete ui; }

void SettingsPage::bindTelemetry(TelemetryData* t) { m_t = t; Q_UNUSED(m_t); }

void SettingsPage::refreshPairedList()
{
    QProcess process;
    process.start("bluetoothctl", QStringList() << "devices");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();

    // On sauvegarde la MAC actuellement sélectionnée pour la restaurer après le rafraîchissement
    QString selectedMac;
    if (ui->listDevices->currentItem()) {
        selectedMac = ui->listDevices->currentItem()->data(Qt::UserRole).toString();
    }

    // Note : On ne compare plus m_lastPairedOutput car l'état de connexion (Connected: yes/no)
    // peut changer sans que la liste des appareils (devices) ne change.
    ui->listDevices->clear();

    QStringList lines = output.split('\n');
    bool found = false;

    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;

        QStringList parts = line.split(' ');
        if (parts.size() >= 3 && parts[0] == "Device") {
            QString mac = parts[1];
            QString name = parts.mid(2).join(' ');

            // --- VÉRIFICATION DE L'ÉTAT DE CONNEXION ---
            QProcess infoProcess;
            infoProcess.start("bluetoothctl", QStringList() << "info" << mac);
            infoProcess.waitForFinished();
            QString infoOutput = infoProcess.readAllStandardOutput();
            bool isConnected = infoOutput.contains("Connected: yes");

            // Enregistrement automatique en "trust" pour les nouveaux appareils
            if (!m_knownMacs.contains(mac)) {
                m_knownMacs.insert(mac);
                QProcess::execute("bluetoothctl", QStringList() << "trust" << mac);

                if (ui->btnVisible->isChecked()) {
                    setDiscoverable(false);
                }
            }

            // Construction du label
            QString label = name + " (" + mac + ")";
            if (isConnected) {
                label += " (connecté)";
            }

            QListWidgetItem *item = new QListWidgetItem("📱 " + label, ui->listDevices);
            item->setData(Qt::UserRole, mac); // Stockage de la MAC pour suppression

            // --- MISE EN COULEUR SI CONNECTÉ ---
            if (isConnected) {
                item->setForeground(Qt::green); // Texte en vert
            }

            // Restaurer la sélection si c'était cet appareil
            if (mac == selectedMac) {
                ui->listDevices->setCurrentItem(item);
            }

            found = true;
        }
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
    // Nettoyage du nom pour la popup (on enlève l'icône et le statut de connexion)
    QString name = item->text().remove("📱 ").remove(" (connecté)");
    if (mac.isEmpty()) return;

    // --- BOITE DE CONFIRMATION ---
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Supprimer l'appareil",
                                  QString("Voulez-vous vraiment oublier l'appareil %1 ?").arg(name),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    // Suppression effective via le système
    QProcess::execute("bluetoothctl", QStringList() << "remove" << mac);

    // Mise à jour immédiate
    m_knownMacs.remove(mac);
    m_lastPairedOutput.clear();
    ui->listDevices->clear();
    ui->btnForget->setEnabled(false);

    refreshPairedList();
}

void SettingsPage::errorOccurred(QBluetoothLocalDevice::Error error)
{
    qDebug() << "Erreur Bluetooth LocalDevice:" << error;
}
