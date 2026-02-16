# Supprime les anciennes versions et mets exactement ceci :
QT += core gui widgets positioning location quickwidgets qml quick serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    androidautopage.cpp \
    camerapage.cpp \
    gpstelemetrysource.cpp \
    homepage.cpp \
    main.cpp \
    mainwindow.cpp \
    mocktelemetrysource.cpp \
    navigationpage.cpp \
    settingspage.cpp \
    telemetrydata.cpp

HEADERS += \
    androidautopage.h \
    camerapage.h \
    gpstelemetrysource.h \
    homepage.h \
    mainwindow.h \
    mocktelemetrysource.h \
    navigationpage.h \
    settingspage.h \
    telemetrydata.h

FORMS += \
    camerapage.ui \
    homepage.ui \
    mainwindow.ui \
    navigationpage.ui \
    settingspage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    map.qml

RESOURCES += \
    resources.qrc


# Bloc spécifique à Linux (Raspberry Pi)
unix:!android {
    DEFINES += Q_OS_LINUX_PI

    # Chemins vers aasdk et protobuf (à adapter selon où vous les avez compilés)
    INCLUDEPATH += /path/to/aasdk/include
    LIBS += -L/path/to/aasdk/lib -laasdk -laasdk_proto

    # Dépendances système
    LIBS += -lprotobuf -lusb-1.0 -lssl -lcrypto -lboost_system

    SOURCES += androidautopage.cpp
    HEADERS += androidautopage.h
}

# Bloc spécifique à Windows
win32 {
    # DEFINES += Q_OS_WIN  <-- SUPPRIMEZ OU COMMENTEZ CETTE LIGNE
    # Ici, on n'ajoute pas les libs Linux
}
