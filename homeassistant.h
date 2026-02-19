#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>

// 1. Nouvelle classe pour écouter ce qu'il se passe sur la page web
class HAPage : public QWebEnginePage {
    Q_OBJECT
public:
    explicit HAPage(QWebEngineProfile* profile, QObject* parent = nullptr);
protected:
    // Cette fonction capte les console.log() du JavaScript
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) override;
signals:
    void showKeyboardRequested();
};

// 2. Ta classe principale (légèrement modifiée)
class HomeAssistant : public QWidget {
    Q_OBJECT
public:
    explicit HomeAssistant(QWidget* parent = nullptr);
    void setUrl(const QString& url);

private slots:
    void openKeyboard(); // <--- La fonction qui ouvre ton clavier custom

private:
    QWebEngineView* m_view;
};

#endif // HOMEASSISTANT_H
