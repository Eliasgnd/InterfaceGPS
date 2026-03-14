QT += core gui widgets positioning quickwidgets qml quick serialport virtualkeyboard multimedia quickcontrols2 network dbus bluetooth webenginewidgets testlib
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = ui_mainwindow_test

SOURCES += \
    tst_ui_mainwindow.cpp \
    ../../mainwindow.cpp \
    ../../navigationpage.cpp \
    ../../camerapage.cpp \
    ../../settingspage.cpp \
    ../../mediapage.cpp \
    ../../homeassistant.cpp \
    ../../clavier.cpp \
    ../../bluetoothmanager.cpp \
    ../../telemetrydata.cpp

HEADERS += \
    ../../mainwindow.h \
    ../../navigationpage.h \
    ../../camerapage.h \
    ../../settingspage.h \
    ../../mediapage.h \
    ../../homeassistant.h \
    ../../clavier.h \
    ../../bluetoothmanager.h \
    ../../telemetrydata.h

FORMS += \
    ../../mainwindow.ui \
    ../../navigationpage.ui \
    ../../camerapage.ui \
    ../../settingspage.ui \
    ../../mediapage.ui

RESOURCES += \
    ../../resources.qrc
