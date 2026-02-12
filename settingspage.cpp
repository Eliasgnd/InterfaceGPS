#include "settingspage.h"
#include "ui_settingspage.h"
#include "telemetrydata.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>

namespace {
    constexpr int SAVE_CONFIRMATION_DURATION_MS = 2000;
}

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::SettingsPage)
{
    ui->setupUi(this);

    // Load saved settings
    loadSettings();

    connect(ui->sliderBrightness, &QSlider::valueChanged, this, [this](int v){
        ui->lblBrightnessValue->setText(QString::number(v));
        emit brightnessChanged(v);
    });

    connect(ui->cmbTheme, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx){
                emit themeChanged(idx);
            });

    connect(ui->btnSave, &QPushButton::clicked, this, [this](){
        saveSettings();
        ui->btnSave->setText("Enregistré ✅");
        QTimer::singleShot(SAVE_CONFIRMATION_DURATION_MS, this, [this](){
            ui->btnSave->setText("Enregistrer");
        });
    });
}

SettingsPage::~SettingsPage(){ delete ui; }

void SettingsPage::bindTelemetry(TelemetryData* t)
{
    m_t = t;
    Q_UNUSED(m_t);
}

void SettingsPage::saveSettings()
{
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataPath.isEmpty()) {
        qWarning() << "Cannot save settings: AppDataLocation is empty";
        return;
    }

    QDir dir;
    if (!dir.mkpath(dataPath)) {
        qWarning() << "Cannot create settings directory:" << dataPath;
        return;
    }

    QJsonObject json;
    json["homeAddress"] = ui->editHome->text().trimmed();
    json["workAddress"] = ui->editWork->text().trimmed();

    const QString filePath = dataPath + "/user_settings.json";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to save settings to:" << filePath;
        return;
    }

    const QJsonDocument doc(json);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Settings saved to:" << filePath;
}

void SettingsPage::loadSettings()
{
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataPath.isEmpty()) {
        qWarning() << "Cannot load settings: AppDataLocation is empty";
        return;
    }

    const QString filePath = dataPath + "/user_settings.json";
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "No saved settings found at:" << filePath;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open settings file:" << filePath;
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "Invalid settings file format at:" << filePath;
        return;
    }

    const QJsonObject json = doc.object();
    ui->editHome->setText(json.value("homeAddress").toString());
    ui->editWork->setText(json.value("workAddress").toString());

    qDebug() << "Settings loaded from:" << filePath;
}

QString SettingsPage::getHomeAddress() const
{
    return ui->editHome->text();
}

QString SettingsPage::getWorkAddress() const
{
    return ui->editWork->text();
}
