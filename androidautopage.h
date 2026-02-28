#ifndef ANDROIDAUTOPAGE_H
#define ANDROIDAUTOPAGE_H

#include <QWidget>
#include <QVBoxLayout>

// On inclut OpenAuto uniquement si on est sous Linux
#ifdef ENABLE_ANDROID_AUTO
#include <fve/UI/QtVideoOutput.hpp>
#endif

class AndroidAutoPage : public QWidget {
    Q_OBJECT
public:
    explicit AndroidAutoPage(QWidget *parent = nullptr);
    ~AndroidAutoPage();

private:
    QVBoxLayout* m_layout;

#ifdef ENABLE_ANDROID_AUTO
    openauto::projection::QtVideoOutput* m_videoOutput = nullptr;
#endif
};

#endif // ANDROIDAUTOPAGE_H
