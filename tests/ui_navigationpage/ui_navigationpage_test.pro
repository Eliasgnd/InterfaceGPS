QT += testlib core gui widgets positioning location quickwidgets qml quick network
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = ui_navigationpage_test

SOURCES += \
    tst_ui_navigationpage.cpp \
    ../../navigationpage.cpp \
    ../../clavier.cpp \
    ../../telemetrydata.cpp

HEADERS += \
    ../../navigationpage.h \
    ../../clavier.h \
    ../../telemetrydata.h

FORMS += \
    ../../navigationpage.ui

RESOURCES += \
    ../../resources.qrc
