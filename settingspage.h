#pragma once
#include <QWidget>
#include <QBluetoothLocalDevice>
#include <QTimer>
#include <QSet> // <--- AJOUT

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
    void onVisibleClicked();
    void onForgetClicked();

    void errorOccurred(QBluetoothLocalDevice::Error error);
    void stopDiscovery();
    void refreshPairedList(); // On va l'appeler avec le timer

private:
    void setDiscoverable(bool enable);

    Ui::SettingsPage* ui;
    TelemetryData* m_t=nullptr;

    QBluetoothLocalDevice *m_localDevice;
    QTimer *m_discoveryTimer;

    // --- NOUVELLES VARIABLES DE SURVEILLANCE ---
    QTimer *m_pollTimer;
    QString m_lastPairedOutput;
    QSet<QString> m_knownMacs;
    //pour 1 seul appareil connecter en même temps
    void showAutoClosingMessage(const QString &title, const QString &text, int timeoutMs);
    QString m_lastActiveMac;


};
