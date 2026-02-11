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
        QTimer::singleShot(2000, this, [this](){
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
    QJsonObject json;
    json["theme"] = ui->cmbTheme->currentIndex();
    json["brightness"] = ui->sliderBrightness->value();
    json["homeAddress"] = ui->editHome->text();
    json["workAddress"] = ui->editWork->text();

    QJsonDocument doc(json);
    
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    QString filePath = dataPath + "/user_settings.json";
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Settings saved to:" << filePath;
    } else {
        qDebug() << "Failed to save settings to:" << filePath;
    }
}

void SettingsPage::loadSettings()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = dataPath + "/user_settings.json";
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No saved settings found at:" << filePath;
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qDebug() << "Invalid settings file format at:" << filePath << "- expected JSON object";
        return;
    }
    
    QJsonObject json = doc.object();
    
    if (json.contains("theme")) {
        ui->cmbTheme->setCurrentIndex(json["theme"].toInt());
    }
    if (json.contains("brightness")) {
        ui->sliderBrightness->setValue(json["brightness"].toInt());
    }
    if (json.contains("homeAddress")) {
        ui->editHome->setText(json["homeAddress"].toString());
    }
    if (json.contains("workAddress")) {
        ui->editWork->setText(json["workAddress"].toString());
    }
    
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
