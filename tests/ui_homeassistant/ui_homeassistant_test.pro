QT += testlib core gui widgets webenginewidgets
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = ui_homeassistant_test

SOURCES += \
    tst_ui_homeassistant.cpp \
    ../../homeassistant.cpp \
    ../../clavier.cpp

HEADERS += \
    ../../homeassistant.h \
    ../../clavier.h
