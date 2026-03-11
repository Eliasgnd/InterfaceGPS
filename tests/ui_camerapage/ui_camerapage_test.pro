QT += testlib core gui widgets network
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = ui_camerapage_test

SOURCES += \
    tst_ui_camerapage.cpp \
    ../../camerapage.cpp

HEADERS += \
    ../../camerapage.h

FORMS += \
    ../../camerapage.ui
