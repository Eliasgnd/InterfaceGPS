#ifndef CLAVIER_H
#define CLAVIER_H

#include <QDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QSettings>
#include <QTimer>
#include <QMap>

class Clavier : public QDialog
{
    Q_OBJECT

public:
    explicit Clavier(QWidget *parent = nullptr);
    QString getText() const;

    // Fonctions pour le lien avec NavigationPage
    void setInitialText(const QString &text);
    void displaySuggestions(const QStringList &suggestions);

signals:
    void textChangedExternally(const QString &text);

private slots:
    void handleButton();
    void deleteChar();
    void validateText();
    void toggleShift();
    void addSpace();
    void addUnderscore();
    void handleLongPress();
    void insertAccent();
    void startDelete();
    void startDeleteDelay();
    void stopDelete();
    void startLongPress();
    void stopLongPress();
    void switchKeyboard();
    void switchLayout();

private:
    QLineEdit *lineEdit;
    QListWidget *suggestionList;
    QGridLayout *gridLayout;

    QPushButton *shiftButton;
    QPushButton *switchButton;
    QPushButton *langueButton;
    QPushButton *apostropheButton;
    QPushButton *currentLongPressButton = nullptr;

    QList<QPushButton*> allButtons;
    QList<QPushButton*> symbolButtons;
    QList<QPushButton*> row1Buttons;
    QList<QPushButton*> row2Buttons;
    QList<QPushButton*> row3Buttons;

    QMap<QString, QStringList> accentMap;
    QTimer *deleteTimer;
    QTimer *deleteDelayTimer;
    QTimer *longPressTimer;
    QWidget *accentPopup = nullptr;
    QWidget *overlay = nullptr;

    enum KeyboardLayout { AZERTY, QWERTY };
    KeyboardLayout currentLayout;

    bool majusculeActive;
    bool isSymbolMode;
    bool shiftLock = false;

    QStringList usageHistory;

    void updateKeys();
    void updateKeyboard();
    void updateKeyboardLayout();
    void hideAccentPopup();
    void showAccentPopup(QPushButton* button);
    void loadUsageHistory();
    void saveUsageHistory() const;
};

#endif // CLAVIER_H
