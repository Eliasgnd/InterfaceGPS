#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H

#include <QWidget>
#include <QWebEngineView>

class HomeAssistant : public QWidget {
    Q_OBJECT
public:
    explicit HomeAssistant(QWidget* parent = nullptr);
    // Permet de changer l'URL dynamiquement si besoin
    void setUrl(const QString& url);

private:
    QWebEngineView* m_view;
};

#endif // HOMEASSISTANT_H
