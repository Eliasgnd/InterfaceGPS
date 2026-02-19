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

    connect(ui->sliderBrightness, &QSlider::valueChanged, this, [this](int v){
        ui->lblBrightnessValue->setText(QString::number(v));
        emit brightnessChanged(v);
    });

    connect(ui->cmbTheme, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx){
                emit themeChanged(idx);
            });


    m_localDevice = new QBluetoothLocalDevice(this);

    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(120000);
    connect(m_discoveryTimer, &QTimer::timeout, this, &SettingsPage::stopDiscovery);

    connect(m_localDevice, &QBluetoothLocalDevice::errorOccurred,
            this, &SettingsPage::errorOccurred);

    connect(ui->btnVisible, &QPushButton::clicked, this, &SettingsPage::onVisibleClicked);
    connect(ui->btnForget, &QPushButton::clicked, this, &SettingsPage::onForgetClicked);

    // --- LE TIMER DE SURVEILLANCE ---
    // Il va vérifier la liste des appareils toutes les 2 secondes
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
    process.start("bluetoothctl", QStringList() << "paired-devices");
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();

    // Si la liste n'a pas changé depuis la dernière seconde, on ne fait rien pour éviter que l'écran clignote
    if (output == m_lastPairedOutput && ui->listDevices->count() > 0) {
        return;
    }

    m_lastPairedOutput = output;
    ui->listDevices->clear();

    QStringList lines = output.split('\n');
    bool found = false;

    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;

        QStringList parts = line.split(' ');
        if (parts.size() >= 3 && parts[0] == "Device") {
            QString mac = parts[1];
            QString name = parts.mid(2).join(' ');

            // Si on détecte un nouvel appareil qu'on ne connaissait pas avant
            if (!m_knownMacs.contains(mac)) {
                m_knownMacs.insert(mac);

                // On s'assure qu'il est en "trust" (au cas où le système l'aurait oublié)
                QProcess::execute("bluetoothctl", QStringList() << "trust" << mac);

                // Si on était en mode "Recherche", on arrête tout !
                if (ui->btnVisible->isChecked()) {
                    setDiscoverable(false);
                }
            }

            QString label = name + " (" + mac + ")";
            QListWidgetItem *item = new QListWidgetItem("📱 " + label, ui->listDevices);
            item->setData(Qt::UserRole, mac);
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
        // Commande système pure pour être visible
        QProcess::execute("bluetoothctl", QStringList() << "discoverable" << "on");
        ui->btnVisible->setText("Visible (120s max)...");
        ui->btnVisible->setChecked(true);
        m_discoveryTimer->start();
    } else {
        // Commande système pure pour être invisible
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
    if (mac.isEmpty()) return;

    // Demande au système de supprimer l'appareil
    QProcess::execute("bluetoothctl", QStringList() << "remove" << mac);

    // On l'enlève de notre liste "connue" et on force le rafraîchissement
    m_knownMacs.remove(mac);
    m_lastPairedOutput.clear();

    refreshPairedList();
}

void SettingsPage::errorOccurred(QBluetoothLocalDevice::Error error)
{
    qDebug() << "Erreur Bluetooth LocalDevice:" << error;
}
