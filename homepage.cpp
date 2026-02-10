#include "homepage.h"
#include "ui_homepage.h"
#include "telemetrydata.h"

HomePage::HomePage(QWidget* parent) : QWidget(parent), ui(new Ui::HomePage) {
    ui->setupUi(this);
    connect(ui->btnNav, &QPushButton::clicked, this, &HomePage::requestNavigation);
    connect(ui->btnCam, &QPushButton::clicked, this, &HomePage::requestCamera);
}
HomePage::~HomePage(){ delete ui; }

void HomePage::bindTelemetry(TelemetryData* t){
    m_t=t;
    if(!m_t) return;

    auto refresh = [this](){
        ui->lblSpeedBig->setText(QString::number((int)qRound(m_t->speedKmh())));
        ui->lblBattery->setText(QString("%1%").arg(m_t->batteryPercent()));
        ui->lblGps->setText(m_t->gpsOk() ? "GPS OK" : "GPS LOST");
        ui->lblReverse->setText(m_t->reverse() ? "R" : "D");
    };
    refresh();

    connect(m_t, &TelemetryData::speedKmhChanged, this, refresh);
    connect(m_t, &TelemetryData::batteryPercentChanged, this, refresh);
    connect(m_t, &TelemetryData::gpsOkChanged, this, refresh);
    connect(m_t, &TelemetryData::reverseChanged, this, refresh);
}
