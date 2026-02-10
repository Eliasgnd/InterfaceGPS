#include "settingspage.h"
#include "ui_settingspage.h"
#include "telemetrydata.h"

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

    connect(ui->btnSave, &QPushButton::clicked, this, [this](){
        // Placeholder: in future you save settings to JSON
        ui->btnSave->setText("Enregistré ✅");
    });
}

SettingsPage::~SettingsPage(){ delete ui; }

void SettingsPage::bindTelemetry(TelemetryData* t)
{
    m_t = t;
    Q_UNUSED(m_t);
}
