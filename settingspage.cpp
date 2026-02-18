#include "settingspage.h"
#include "ui_settingspage.h"
#include "telemetrydata.h"
#include <QMessageBox>
#include <QDebug>

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::SettingsPage)
{
    ui->setupUi(this);

    // --- Connexions existantes ---
    connect(ui->sliderBrightness, &QSlider::valueChanged, this, [this](int v){
        ui->lblBrightnessValue->setText(QString::number(v));
        emit brightnessChanged(v);
    });

    connect(ui->cmbTheme, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx){
                emit themeChanged(idx);
            });

    // --- INITIALISATION BLUETOOTH ---
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_localDevice = new QBluetoothLocalDevice(this);

    // On rend le PC/Raspberry visible pour les autres (optionnel)
    m_localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);

    // Connexions Scan
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &SettingsPage::deviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &SettingsPage::scanFinished);

    // Connexions Appairage
    connect(m_localDevice, &QBluetoothLocalDevice::pairingFinished,
            this, &SettingsPage::pairingFinished);
    connect(m_localDevice, &QBluetoothLocalDevice::errorOccurred,
            this, &SettingsPage::pairingError);

    // Connexions Boutons (Assure-toi d'avoir créé ces boutons dans le .ui !)
    // Si tu n'as pas encore créé les boutons dans le .ui, ces lignes feront planter la compilation.
    // Décommente-les une fois l'UI mise à jour.
    connect(ui->btnScan, &QPushButton::clicked, this, &SettingsPage::onScanClicked);
    connect(ui->btnPair, &QPushButton::clicked, this, &SettingsPage::onPairClicked);
}

SettingsPage::~SettingsPage(){ delete ui; }

void SettingsPage::bindTelemetry(TelemetryData* t)
{
    m_t = t;
    Q_UNUSED(m_t);
}

// --- GESTION BLUETOOTH ---

void SettingsPage::onScanClicked()
{
    ui->listDevices->clear();
    ui->btnScan->setText("Scan en cours...");
    ui->btnScan->setEnabled(false);

    // On lance la recherche (Classic Bluetooth + BLE)
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void SettingsPage::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    QString name = device.name();
    if (name.isEmpty()) return; // On ignore les appareils sans nom

    QString address = device.address().toString();
    QString label = name + " (" + address + ")";

    // On vérifie si on l'a déjà dans la liste pour éviter les doublons
    QList<QListWidgetItem*> items = ui->listDevices->findItems(label, Qt::MatchFixedString);
    if (items.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem(label, ui->listDevices);

        // On stocke l'adresse MAC et l'objet DeviceInfo dans l'item pour s'en servir après
        item->setData(Qt::UserRole, address);

        // Optionnel : Mettre en vert si déjà appairé
        if (m_localDevice->pairingStatus(device.address()) == QBluetoothLocalDevice::Paired) {
            item->setForeground(QColor("#4CAF50")); // Vert
            item->setText(label + " [Connecté]");
        }
    }
}

void SettingsPage::scanFinished()
{
    ui->btnScan->setText("Scanner");
    ui->btnScan->setEnabled(true);
}

void SettingsPage::onPairClicked()
{
    QListWidgetItem *item = ui->listDevices->currentItem();
    if (!item) return;

    QString addressStr = item->data(Qt::UserRole).toString();
    QBluetoothAddress address(addressStr);

    // On lance la demande d'appairage
    // Note : Sur Linux/BlueZ, cela déclenchera généralement une notification système
    // ou gérera l'appairage automatiquement si c'est un appareil audio.
    m_localDevice->requestPairing(address, QBluetoothLocalDevice::Paired);

    ui->btnPair->setText("Connexion...");
    ui->btnPair->setEnabled(false);
}

void SettingsPage::pairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing status)
{
    Q_UNUSED(address);
    ui->btnPair->setEnabled(true);

    if (status == QBluetoothLocalDevice::Paired) {
        ui->btnPair->setText("Appairé !");
        // On relance un scan rapide ou on rafraichit la liste pour afficher en vert
        onScanClicked();
    } else {
        ui->btnPair->setText("Connecter");
    }
}

void SettingsPage::pairingError(QBluetoothLocalDevice::Error error)
{
    ui->btnPair->setEnabled(true);
    ui->btnPair->setText("Erreur");
    qDebug() << "Erreur Bluetooth:" << error;
}
