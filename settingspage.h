#pragma once
#include <QWidget>
#include <QBluetoothLocalDevice>
#include <QTimer>

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
    // Slots de l'interface
    void onVisibleClicked();
    void onForgetClicked();

    // Slots Bluetooth
    void pairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing status);
    void errorOccurred(QBluetoothLocalDevice::Error error);
    void stopDiscovery(); // Appelé auto par le timer pour cacher le bluetooth

private:
    void refreshPairedList();
    void setDiscoverable(bool enable);

    Ui::SettingsPage* ui;
    TelemetryData* m_t=nullptr;

    QBluetoothLocalDevice *m_localDevice;
    QTimer *m_discoveryTimer; // Pour couper la visibilité auto
};
