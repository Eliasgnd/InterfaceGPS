#include "androidautopage.h"
#include <QLabel>

AndroidAutoPage::AndroidAutoPage(QWidget *parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

#ifdef ENABLE_ANDROID_AUTO
    // Si on est sur le Pi, on prépare la zone vidéo d'OpenAuto
    m_videoOutput = new openauto::projection::QtVideoOutput(this);
    m_layout->addWidget(m_videoOutput->getWidget());
#else
    // Si on est sur Windows/Mac, on affiche un simple texte
    QLabel* label = new QLabel("Interface Android Auto\n(Disponible uniquement sur Raspberry Pi)", this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: white; font-size: 24px;");
    m_layout->addWidget(label);
#endif

    this->setStyleSheet("background-color: black;");
}

AndroidAutoPage::~AndroidAutoPage() {}
