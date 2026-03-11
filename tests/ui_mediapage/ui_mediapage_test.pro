QT += testlib core gui widgets quickwidgets qml quick dbus bluetooth quickcontrols2
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = ui_mediapage_test

SOURCES += \
    tst_ui_mediapage.cpp \
    ../../mediapage.cpp \
    ../../bluetoothmanager.cpp

HEADERS += \
    ../../mediapage.h \
    ../../bluetoothmanager.h

FORMS += \
    ../../mediapage.ui

RESOURCES += \
    ../../resources.qrc
