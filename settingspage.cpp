#include "settingspage.h"
#include "ui_settingspage.h"
#include "telemetrydata.h"
#include <QMessageBox>
#include <QDebug>

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::SettingsPage)
{
    ui->setupUi(this);

    // --- SETUP UI (Luminosité & Thème) ---
    connect(ui->sliderBrightness, &QSlider::valueChanged, this, [this](int v){
        ui->lblBrightnessValue->setText(QString::number(v));
        emit brightnessChanged(v);
    });

    connect(ui->cmbTheme, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx){
                emit themeChanged(idx);
            });


    // --- SETUP BLUETOOTH ---
    m_localDevice = new QBluetoothLocalDevice(this);

    // Par défaut : INVISIBLE mais CONNECTABLE (pour les téléphones déjà connus)
    m_localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);

    // Timer pour couper la visibilité automatiquement après 2 minutes (sécurité)
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(120000); // 120 secondes = 2 minutes
    connect(m_discoveryTimer, &QTimer::timeout, this, &SettingsPage::stopDiscovery);

    // Connexions Bluetooth système
    connect(m_localDevice, &QBluetoothLocalDevice::pairingFinished,
            this, &SettingsPage::pairingFinished);
    connect(m_localDevice, &QBluetoothLocalDevice::errorOccurred,
            this, &SettingsPage::errorOccurred);

    // Connexions Boutons
    connect(ui->btnVisible, &QPushButton::clicked, this, &SettingsPage::onVisibleClicked);
    connect(ui->btnForget, &QPushButton::clicked, this, &SettingsPage::onForgetClicked);

    // On remplit la liste au démarrage
    refreshPairedList();
}

SettingsPage::~SettingsPage(){ delete ui; }

void SettingsPage::bindTelemetry(TelemetryData* t)
{
    m_t = t;
    Q_UNUSED(m_t);
}

// --- LOGIQUE BLUETOOTH ---

void SettingsPage::refreshPairedList()
{
    ui->listDevices->clear();

    // Récupère la liste des appareils déjà appairés (enregistrés par le système)
    QList<QBluetoothAddress> pairedAddresses = m_localDevice->pairedDevices();

    for (const QBluetoothAddress &addr : pairedAddresses) {
        // On essaie de récupérer le statut. Le nom n'est pas toujours dispo hors connexion.
        // On affiche l'adresse MAC pour être sûr d'identifier l'appareil.
        QString label = addr.toString();

        QListWidgetItem *item = new QListWidgetItem("📱 Appareil (" + label + ")", ui->listDevices);
        item->setData(Qt::UserRole, label); // On stocke l'adresse pour pouvoir le supprimer
    }

    if (ui->listDevices->count() == 0) {
        new QListWidgetItem("(Aucun appareil enregistré)", ui->listDevices);
        ui->btnForget->setEnabled(false);
    } else {
        ui->btnForget->setEnabled(true);
    }
}

void SettingsPage::onVisibleClicked()
{
    // Si le bouton est coché (enfoncé), on devient visible
    if (ui->btnVisible->isChecked()) {
        setDiscoverable(true);
    } else {
        // Sinon on redevient invisible manuellement
        setDiscoverable(false);
    }
}

void SettingsPage::setDiscoverable(bool enable)
{
    if (enable) {
        // On devient VISIBLE pour tous
        m_localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);
        ui->btnVisible->setText("Visible (120s max)...");
        ui->btnVisible->setChecked(true);
        // On lance le chrono
        m_discoveryTimer->start();
    } else {
        // On redevient INVISIBLE (mais connectable par ceux qui nous connaissent)
        m_localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);
        ui->btnVisible->setText("Rendre Visible (Appairage)");
        ui->btnVisible->setChecked(false);
        m_discoveryTimer->stop();
    }
}

void SettingsPage::stopDiscovery()
{
    // Appelé automatiquement par le timer quand le temps est écoulé
    setDiscoverable(false);
}

void SettingsPage::pairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing status)
{
    Q_UNUSED(address);
    if (status == QBluetoothLocalDevice::Paired) {
        // C'est ici qu'on "dégage" les autres :
        // Dès qu'un téléphone a fini de se connecter, on coupe la visibilité !
        setDiscoverable(false);

        // On met à jour la liste pour voir le nouveau téléphone
        refreshPairedList();

        QMessageBox::information(this, "Bluetooth", "Appareil connecté avec succès !");
    }
}

void SettingsPage::onForgetClicked()
{
    QListWidgetItem *item = ui->listDevices->currentItem();
    if (!item) return;

    QString addressStr = item->data(Qt::UserRole).toString();
    if (addressStr.isEmpty()) return; // Cas du message "(Aucun appareil)"

    QBluetoothAddress address(addressStr);

    // On supprime l'appairage (Oublier l'appareil)
    m_localDevice->requestUnpairing(address, QBluetoothLocalDevice::Unpaired);

    // On enlève de la liste visuelle
    refreshPairedList();
}

void SettingsPage::errorOccurred(QBluetoothLocalDevice::Error error)
{
    qDebug() << "Erreur Bluetooth LocalDevice:" << error;
}
