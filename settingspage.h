#pragma once
#include <QWidget>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceInfo>

namespace Ui { class SettingsPage; }
class TelemetryData;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent=nullptr);
    ~SettingsPage();
    void bindTelemetry(TelemetryData* t);

signals:
    void themeChanged(int themeIndex);
    void brightnessChanged(int value);

private slots:
    // Slots pour l'UI
    void onScanClicked();
    void onPairClicked();

    // Slots Bluetooth
    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void scanFinished();
    void pairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing status);
    void pairingError(QBluetoothLocalDevice::Error error);

private:
    Ui::SettingsPage* ui;
    TelemetryData* m_t=nullptr;

    // Objets Bluetooth
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QBluetoothLocalDevice *m_localDevice;
};
