#pragma once
#include <QWidget>

class AndroidAutoPage : public QWidget {
    Q_OBJECT
public:
    explicit AndroidAutoPage(QWidget* parent = nullptr);
    void startEmulation();
};
