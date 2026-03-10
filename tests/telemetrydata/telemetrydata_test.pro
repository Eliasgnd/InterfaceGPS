QT += testlib core positioning serialport
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = telemetrydata_test

SOURCES += \
    tst_telemetrydata.cpp \
    ../../telemetrydata.cpp \
    ../../gpstelemetrysource.cpp \
    ../../mpu9250source.cpp

HEADERS += \
    ../../telemetrydata.h \
    ../../gpstelemetrysource.h \
    ../../mpu9250source.h
