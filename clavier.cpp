// Rôle architectural: implémentation du clavier virtuel transverse à l'application.
// Responsabilités: orchestrer les interactions tactiles, les accents, la suppression continue et l'historique local.
// Dépendances principales: Qt Widgets, signaux/slots, QTimer et persistance QSettings.

#include "clavier.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>

Clavier::Clavier(QWidget *parent) : QDialog(parent), majusculeActive(true), isSymbolMode(false)
{
    setWindowTitle("Clavier GPS");


    setFixedSize(1000, 750);
    currentLayout = AZERTY;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);



    mainLayout->addStretch(1);

    suggestionList = new QListWidget(this);
    suggestionList->setVisible(false);
    suggestionList->setMaximumHeight(160);
    suggestionList->setStyleSheet("font-size: 20px; background: white; color: black; border-radius: 10px;");
    mainLayout->addWidget(suggestionList);

    lineEdit = new QLineEdit(this);
    lineEdit->setFixedHeight(65);
    lineEdit->setStyleSheet("font-size: 24px; padding: 10px; font-weight: bold; background: #2a2f3a; color: white; border-radius: 8px;");
    mainLayout->addWidget(lineEdit);


    connect(suggestionList, &QListWidget::itemClicked, this, [=](QListWidgetItem *item){
        lineEdit->setText(item->text());
        suggestionList->hide();
        emit textChangedExternally(lineEdit->text());
    });

    gridLayout = new QGridLayout();
    gridLayout->setSpacing(8);


    int btnW = 85;
    int btnH = 80;
    QString keyStyle = "QPushButton { font-size: 22px; font-weight: bold; border-radius: 8px; background: #333a4a; color: white; } "
                       "QPushButton:pressed { background: #4a5468; }";


    QStringList chiffres = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int i = 0; i < chiffres.size(); ++i) {
        QPushButton *btn = new QPushButton(chiffres[i], this);
        btn->setFixedSize(btnW, btnH);
        btn->setStyleSheet(keyStyle);
        connect(btn, &QPushButton::clicked, this, &Clavier::handleButton);
        gridLayout->addWidget(btn, 0, i);
    }


    QPushButton *btnDel = new QPushButton("⌫", this);
    btnDel->setFixedSize(110, btnH);
    btnDel->setStyleSheet("QPushButton { font-size: 26px; background: #852222; color: white; border-radius: 8px; font-weight: bold; }");
    connect(btnDel, &QPushButton::pressed, this, &Clavier::startDeleteDelay);
    connect(btnDel, &QPushButton::released, this, &Clavier::stopDelete);
    gridLayout->addWidget(btnDel, 0, 10);


    QStringList r1 = {"A", "Z", "E", "R", "T", "Y", "U", "I", "O", "P"};
    QStringList r2 = {"Q", "S", "D", "F", "G", "H", "J", "K", "L", "M"};
    QStringList r3 = {"W", "X", "C", "V", "B", "N"};

    auto createRow = [&](const QStringList& keys, int rowIdx, QList<QPushButton*>& list, int colOffset = 0) {
        for (int i = 0; i < keys.size(); ++i) {
            QPushButton *btn = new QPushButton(keys[i], this);
            btn->setFixedSize(btnW, btnH);
            btn->setStyleSheet(keyStyle);
            list.append(btn);
            allButtons.append(btn);
            connect(btn, &QPushButton::pressed, this, &Clavier::startLongPress);
            connect(btn, &QPushButton::released, this, &Clavier::stopLongPress);
            connect(btn, &QPushButton::clicked, this, &Clavier::handleButton);
            gridLayout->addWidget(btn, rowIdx, i + colOffset);
        }
    };

    createRow(r1, 1, row1Buttons);
    createRow(r2, 2, row2Buttons);
    createRow(r3, 3, row3Buttons, 1);

    apostropheButton = new QPushButton("'", this);
    apostropheButton->setFixedSize(btnW, btnH);
    apostropheButton->setStyleSheet(keyStyle);
    allButtons.append(apostropheButton);
    connect(apostropheButton, &QPushButton::clicked, this, &Clavier::handleButton);
    gridLayout->addWidget(apostropheButton, 3, 8);

    shiftButton = new QPushButton("⇧", this);
    shiftButton->setFixedSize(110, btnH);
    shiftButton->setStyleSheet(keyStyle);
    connect(shiftButton, &QPushButton::clicked, this, &Clavier::toggleShift);
    gridLayout->addWidget(shiftButton, 3, 9, 1, 2);


    langueButton = new QPushButton("🌍 AZERTY", this);
    langueButton->setFixedSize(140, btnH);
    langueButton->setStyleSheet(keyStyle);
    connect(langueButton, &QPushButton::clicked, this, &Clavier::switchLayout);
    gridLayout->addWidget(langueButton, 4, 0, 1, 2);

    switchButton = new QPushButton("123?", this);
    switchButton->setFixedSize(100, btnH);
    switchButton->setStyleSheet(keyStyle);
    connect(switchButton, &QPushButton::clicked, this, &Clavier::switchKeyboard);
    gridLayout->addWidget(switchButton, 4, 2);

    QPushButton *btnSpace = new QPushButton("ESPACE", this);
    btnSpace->setFixedSize(350, btnH);
    btnSpace->setStyleSheet(keyStyle);
    connect(btnSpace, &QPushButton::clicked, this, &Clavier::addSpace);
    gridLayout->addWidget(btnSpace, 4, 3, 1, 4);

    QPushButton *btnUnderscore = new QPushButton("_", this);
    btnUnderscore->setFixedSize(btnW, btnH);
    btnUnderscore->setStyleSheet(keyStyle);
    connect(btnUnderscore, &QPushButton::clicked, this, &Clavier::addUnderscore);
    gridLayout->addWidget(btnUnderscore, 4, 7);

    QPushButton *btnVal = new QPushButton("✔", this);
    btnVal->setFixedSize(140, btnH);
    btnVal->setStyleSheet("QPushButton { background-color: #2e7d32; color: white; font-size: 28px; font-weight: bold; border-radius: 8px; }");
    connect(btnVal, &QPushButton::clicked, this, &Clavier::validateText);
    gridLayout->addWidget(btnVal, 4, 9, 1, 2);


    QStringList syms = {"@","#","$","%","&","*","(",")","-","+","=","/","\\","{","}","[","]",";",":","\"","<",">",",",".","?","!","|"};
    int sRow = 1, sCol = 0;
    for (const QString &s : syms) {
        QPushButton *sb = new QPushButton(s, this);
        sb->setFixedSize(btnW, btnH);
        sb->setStyleSheet(keyStyle);
        sb->setVisible(false);
        symbolButtons.append(sb);
        connect(sb, &QPushButton::clicked, this, &Clavier::handleButton);
        gridLayout->addWidget(sb, sRow, sCol++);
        if (sCol > 9) { sCol = 0; sRow++; }
    }

    mainLayout->addLayout(gridLayout);

    deleteTimer = new QTimer(this);
    deleteDelayTimer = new QTimer(this);
    deleteDelayTimer->setSingleShot(true);
    connect(deleteDelayTimer, &QTimer::timeout, this, &Clavier::startDelete);
    connect(deleteTimer, &QTimer::timeout, this, &Clavier::deleteChar);

    longPressTimer = new QTimer(this);
    longPressTimer->setSingleShot(true);
    connect(longPressTimer, &QTimer::timeout, this, &Clavier::handleLongPress);

    accentMap.insert("e", {"é", "è", "ê", "ë"});
    accentMap.insert("a", {"à", "â"});
    accentMap.insert("i", {"î", "ï"});
    accentMap.insert("o", {"ô", "ö"});
    accentMap.insert("u", {"û", "ù"});
    accentMap.insert("c", {"ç"});

    loadUsageHistory();
    updateKeyboardLayout();
}

void Clavier::handleButton() {
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (btn) {
        lineEdit->insert(btn->text());
        emit textChangedExternally(lineEdit->text());
        if (majusculeActive && !shiftLock) { majusculeActive = false; updateKeys(); }
    }
}
void Clavier::deleteChar() { lineEdit->backspace(); emit textChangedExternally(lineEdit->text()); }
void Clavier::validateText() { saveUsageHistory(); accept(); }
void Clavier::toggleShift() {
    if (!majusculeActive) { majusculeActive = true; shiftLock = false; }
    else if (!shiftLock) { shiftLock = true; }
    else { majusculeActive = false; shiftLock = false; }
    updateKeys();
}
void Clavier::updateKeys() {
    for (QPushButton *btn : allButtons) {
        QString t = btn->text();
        btn->setText((majusculeActive || shiftLock) ? t.toUpper() : t.toLower());
    }
}
void Clavier::addSpace() { lineEdit->insert(" "); emit textChangedExternally(lineEdit->text()); }
void Clavier::addUnderscore() { lineEdit->insert("_"); emit textChangedExternally(lineEdit->text()); }
void Clavier::switchKeyboard() {
    isSymbolMode = !isSymbolMode;
    for (QPushButton *b : allButtons) b->setVisible(!isSymbolMode);
    for (QPushButton *b : symbolButtons) b->setVisible(isSymbolMode);
    switchButton->setText(isSymbolMode ? "ABC" : "123?");
}
void Clavier::switchLayout() {
    currentLayout = (currentLayout == AZERTY) ? QWERTY : AZERTY;
    updateKeyboardLayout();
}
void Clavier::updateKeyboardLayout() {
    QStringList n1, n2, n3;
    if (currentLayout == AZERTY) {
        n1 = {"A", "Z", "E", "R", "T", "Y", "U", "I", "O", "P"};
        n2 = {"Q", "S", "D", "F", "G", "H", "J", "K", "L", "M"};
        n3 = {"W", "X", "C", "V", "B", "N"};
        langueButton->setText("🌍 AZERTY");
    } else {
        n1 = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"};
        n2 = {"A", "S", "D", "F", "G", "H", "J", "K", "L", ";"};
        n3 = {"Z", "X", "C", "V", "B", "N", "M"};
        langueButton->setText("🌍 QWERTY");
    }
    for (int i=0; i<row1Buttons.size() && i<n1.size(); ++i) row1Buttons[i]->setText(n1[i]);
    for (int i=0; i<row2Buttons.size() && i<n2.size(); ++i) row2Buttons[i]->setText(n2[i]);
    for (int i=0; i<row3Buttons.size() && i<n3.size(); ++i) row3Buttons[i]->setText(n3[i]);
    updateKeys();
}
void Clavier::startLongPress() { currentLongPressButton = qobject_cast<QPushButton*>(sender()); longPressTimer->start(500); }
void Clavier::stopLongPress() { longPressTimer->stop(); }
void Clavier::handleLongPress() { if (currentLongPressButton && accentMap.contains(currentLongPressButton->text().toLower())) showAccentPopup(currentLongPressButton); }
void Clavier::showAccentPopup(QPushButton* btn) {
    if (accentPopup) return;
    accentPopup = new QWidget(this, Qt::Popup);
    accentPopup->setAttribute(Qt::WA_DeleteOnClose);
    QHBoxLayout *l = new QHBoxLayout(accentPopup);
    QStringList accs = accentMap.value(btn->text().toLower());
    for (const QString &a : accs) {
        QPushButton *ab = new QPushButton(majusculeActive ? a.toUpper() : a, accentPopup);
        ab->setFixedSize(50, 50);
        connect(ab, &QPushButton::clicked, this, &Clavier::insertAccent);
        l->addWidget(ab);
    }
    accentPopup->move(btn->mapToGlobal(QPoint(0, -60)));
    accentPopup->show();
}
void Clavier::insertAccent() {
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (b) { lineEdit->insert(b->text()); emit textChangedExternally(lineEdit->text()); }
    if (accentPopup) { accentPopup->close(); accentPopup = nullptr; }
}
void Clavier::startDeleteDelay() { deleteChar(); deleteDelayTimer->start(500); }
void Clavier::startDelete() { deleteTimer->start(75); }
void Clavier::stopDelete() { deleteTimer->stop(); deleteDelayTimer->stop(); }
void Clavier::loadUsageHistory() { QSettings s("EliasCorp", "GPSApp"); usageHistory = s.value("Clavier/UsageHistory").toStringList(); }
void Clavier::saveUsageHistory() const { QSettings s("EliasCorp", "GPSApp"); s.setValue("Clavier/UsageHistory", usageHistory); }
QString Clavier::getText() const { return lineEdit->text(); }
void Clavier::setInitialText(const QString &text) { lineEdit->setText(text); lineEdit->setCursorPosition(text.length()); }
void Clavier::displaySuggestions(const QStringList &suggestions) {
    suggestionList->clear();
    if (!suggestions.isEmpty()) { suggestionList->addItems(suggestions); suggestionList->show(); }
    else { suggestionList->hide(); }
}
void Clavier::hideAccentPopup() { if (accentPopup) accentPopup->hide(); }
