// Rôle architectural: intégration embarquée de l'interface Home Assistant.
// Responsabilités: encapsuler la vue web et relayer les demandes d'ouverture de clavier vers l'application Qt.
// Dépendances principales: QWebEngineView, QWebEnginePage et signalisation interne UI.

#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebEnginePage>

class HAPage : public QWebEnginePage {
    Q_OBJECT
public:
    explicit HAPage(QWebEngineProfile* profile, QObject* parent = nullptr);
protected:

    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) override;
signals:
    void showKeyboardRequested();
};

class HomeAssistant : public QWidget {
    Q_OBJECT
public:
    explicit HomeAssistant(QWidget* parent = nullptr);
    void setUrl(const QString& url);

private slots:
    void openKeyboard();

private:
    QWebEngineView* m_view;
};

#endif
