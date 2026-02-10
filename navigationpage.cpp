#include "navigationpage.h"
#include "ui_navigationpage.h"
#include "telemetrydata.h"

NavigationPage::NavigationPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::NavigationPage)
{
    ui->setupUi(this);

    connect(ui->btnZoomIn, &QPushButton::clicked, this, [this](){
        m_zoom = qMin(20, m_zoom + 1);
        ui->lblMap->setText(QString("Carte (OSM) - placeholder (zoom %1)").arg(m_zoom));
    });
    connect(ui->btnZoomOut, &QPushButton::clicked, this, [this](){
        m_zoom = qMax(1, m_zoom - 1);
        ui->lblMap->setText(QString("Carte (OSM) - placeholder (zoom %1)").arg(m_zoom));
    });
    connect(ui->btnCenter, &QPushButton::clicked, this, [this](){
        ui->lblMap->setText(QString("Carte (OSM) - placeholder (centré, zoom %1)").arg(m_zoom));
    });
}

NavigationPage::~NavigationPage(){ delete ui; }

void NavigationPage::bindTelemetry(TelemetryData* t)
{
    m_t = t;
    if(!m_t) return;

    auto refresh = [this](){
        ui->lblLat->setText(QString("lat: %1").arg(m_t->lat(), 0, 'f', 6));
        ui->lblLon->setText(QString("lon: %1").arg(m_t->lon(), 0, 'f', 6));
    };
    refresh();
    connect(m_t, &TelemetryData::latChanged, this, refresh);
    connect(m_t, &TelemetryData::lonChanged, this, refresh);
}
