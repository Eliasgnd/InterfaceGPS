QT += testlib core gui widgets bluetooth
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = ui_settingspage_test

SOURCES += \
    tst_ui_settingspage.cpp \
    ../../settingspage.cpp

HEADERS += \
    ../../settingspage.h

FORMS += \
    ../../settingspage.ui
