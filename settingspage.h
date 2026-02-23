// Rôle architectural: page de configuration système et Bluetooth utilisateur.
// Responsabilités: piloter les préférences UI et administrer la liste des périphériques appairés.
// Dépendances principales: QWidget, QProcess, QTimer et widgets de formulaire Qt.

#pragma once
#include <QWidget>
#include <QBluetoothLocalDevice>
#include <QTimer>
#include <QSet>

namespace Ui { class SettingsPage; }
class TelemetryData;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent=nullptr);
    ~SettingsPage();
signals:
    void themeChanged(int themeIndex);
    void brightnessChanged(int value);

private slots:
    void onVisibleClicked();
    void onForgetClicked();

    void errorOccurred(QBluetoothLocalDevice::Error error);
    void stopDiscovery();
    void refreshPairedList();

private:
    void setDiscoverable(bool enable);

    Ui::SettingsPage* ui;

    QBluetoothLocalDevice *m_localDevice;
    QTimer *m_discoveryTimer;


    QTimer *m_pollTimer;
    QSet<QString> m_knownMacs;

    void showAutoClosingMessage(const QString &title, const QString &text, int timeoutMs);
    QString m_lastActiveMac;


};
