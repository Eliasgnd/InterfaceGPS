#include "camerapage.h"
#include "ui_camerapage.h"
#include "telemetrydata.h"

CameraPage::CameraPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::CameraPage)
{
    ui->setupUi(this);

    connect(ui->btnRear, &QPushButton::clicked, this, [this](){ setMode("REAR"); });
    connect(ui->btnFront, &QPushButton::clicked, this, [this](){ setMode("FRONT"); });
    connect(ui->btnMosaic, &QPushButton::clicked, this, [this](){ setMode("MOSAIC"); });
    connect(ui->btnBird, &QPushButton::clicked, this, [this](){ setMode("BIRD-EYE"); });
}

CameraPage::~CameraPage(){ delete ui; }

void CameraPage::setMode(const QString& mode)
{
    ui->lblMode->setText(QString("Mode: %1").arg(mode));
}

void CameraPage::bindTelemetry(TelemetryData* t)
{
    m_t = t;
    if(!m_t) return;

    auto refresh = [this](){
        // petit feedback visuel dans le placeholder si reverse
        if(m_t->reverse()){
            ui->lblVideo->setText("Flux caméra / Bird-eye - placeholder\n(REVERSE engagé)");
            ui->lblVideo->setStyleSheet("QLabel{color:#FFD27C; font-size:16px;}");
        } else {
            ui->lblVideo->setText("Flux caméra / Bird-eye - placeholder");
            ui->lblVideo->setStyleSheet("QLabel{color:#b8c0cc; font-size:16px;}");
        }
    };
    refresh();
    connect(m_t, &TelemetryData::reverseChanged, this, refresh);
}
