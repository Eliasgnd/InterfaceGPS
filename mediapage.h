#pragma once
#include <QWidget>
#include <QQuickWidget>

namespace Ui { class MediaPage; }

class MediaPage : public QWidget {
    Q_OBJECT
public:
    explicit MediaPage(QWidget* parent=nullptr);
    ~MediaPage();

private:
    Ui::MediaPage* ui;
    QQuickWidget* m_playerView;
};
