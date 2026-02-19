#include "settingspage.h"
#include "ui_settingspage.h"
#include "telemetrydata.h"
#include <QMessageBox>
#include <QDebug>
#include <QProcess> // <--- Ajouté pour lancer bluetoothctl

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

    // Timer pour couper la visibilité automatiquement après 2 minutes
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(120000); // 120 secondes
    connect(m_discoveryTimer, &QTimer::timeout, this, &SettingsPage::stopDiscovery);

    // Connexions Bluetooth
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

    // Sur Qt6 Linux, pairedDevices() n'est plus dispo directement dans QBluetoothLocalDevice.
    // On utilise l'outil système 'bluetoothctl' qui est standard sur Raspberry Pi OS.
    QProcess process;
    process.start("bluetoothctl", QStringList() << "paired-devices");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');

    bool found = false;
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;

        // Format attendu: "Device XX:XX:XX:XX:XX:XX Nom du téléphone"
        QStringList parts = line.split(' ');
        if (parts.size() >= 3 && parts[0] == "Device") {
            QString mac = parts[1];
            QString name = parts.mid(2).join(' '); // Le reste est le nom

            QString label = name + " (" + mac + ")";
            QListWidgetItem *item = new QListWidgetItem("📱 " + label, ui->listDevices);
            item->setData(Qt::UserRole, mac); // On garde l'adresse MAC cachée
            found = true;
        }
    }

    if (!found) {
        new QListWidgetItem("(Aucun appareil enregistré)", ui->listDevices);
        ui->btnForget->setEnabled(false);
    } else {
        ui->btnForget->setEnabled(true);
    }
}

void SettingsPage::onVisibleClicked()
{
    if (ui->btnVisible->isChecked()) {
        setDiscoverable(true);
    } else {
        setDiscoverable(false);
    }
}

void SettingsPage::setDiscoverable(bool enable)
{
    if (enable) {
        // Au lieu d'utiliser Qt, on utilise la vraie commande système qui marche à 100%
        QProcess::execute("bluetoothctl", QStringList() << "discoverable" << "on");

        ui->btnVisible->setText("Visible (120s max)...");
        ui->btnVisible->setChecked(true);
        m_discoveryTimer->start();
    } else {
        // On coupe la visibilité via le système
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

void SettingsPage::pairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing status)
{
    if (status == QBluetoothLocalDevice::Paired) {
        // 1. On redevient invisible
        setDiscoverable(false);

        // 2. SUR LINUX : Il faut absolument "Trust" (Faire confiance) à l'appareil
        // sinon la connexion audio (A2DP) va bloquer ou timeout.
        QProcess::execute("bluetoothctl", QStringList() << "trust" << address.toString());

        // Optionnel : On peut même forcer la connexion dans la foulée
        // QProcess::execute("bluetoothctl", QStringList() << "connect" << address.toString());

        // 3. On met à jour l'affichage
        refreshPairedList();

        QMessageBox::information(this, "Bluetooth", "Appareil connecté et autorisé avec succès !");
    }
}

void SettingsPage::onForgetClicked()
{
    QListWidgetItem *item = ui->listDevices->currentItem();
    if (!item) return;

    QString addressStr = item->data(Qt::UserRole).toString();
    if (addressStr.isEmpty()) return;

    QBluetoothAddress address(addressStr);

    // CORRECTION QT6 : Pour "oublier", on demande un appairage avec le statut "Unpaired"
    m_localDevice->requestPairing(address, QBluetoothLocalDevice::Unpaired);

    // Petit délai pour laisser le système traiter la commande, puis on rafraichit
    // (Ou on supprime juste la ligne pour l'instant pour que ce soit réactif)
    delete item;

    if (ui->listDevices->count() == 0) {
        new QListWidgetItem("(Aucun appareil enregistré)", ui->listDevices);
        ui->btnForget->setEnabled(false);
    }
}

void SettingsPage::errorOccurred(QBluetoothLocalDevice::Error error)
{
    qDebug() << "Erreur Bluetooth LocalDevice:" << error;
}
