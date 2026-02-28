# Supprime les anciennes versions et mets exactement ceci :
QT += core gui widgets positioning location quickwidgets qml quick serialport virtualkeyboard multimedia quickcontrols2 network dbus bluetooth webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    androidautopage.cpp \
    bluetoothmanager.cpp \
    camerapage.cpp \
    clavier.cpp \
    gpstelemetrysource.cpp \
    homeassistant.cpp \
    main.cpp \
    mainwindow.cpp \
    mediapage.cpp \
    navigationpage.cpp \
    settingspage.cpp \
    telemetrydata.cpp

HEADERS += \
    androidautopage.h \
    bluetoothmanager.h \
    camerapage.h \
    clavier.h \
    gpstelemetrysource.h \
    homeassistant.h \
    mainwindow.h \
    mediapage.h \
    navigationpage.h \
    settingspage.h \
    telemetrydata.h

FORMS += \
    camerapage.ui \
    mainwindow.ui \
    mediapage.ui \
    navigationpage.ui \
    settingspage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    MediaPlayer.qml \
    map.qml

RESOURCES += \
    resources.qrc

# --- MODULE ANDROID AUTO (PI 4 / LINUX UNIQUEMENT) ---
linux {
    message("Architecture Linux détectée : Activation du module Android Auto")

    # Cette variable permet d'activer le code C++ spécifique
    DEFINES += ENABLE_ANDROID_AUTO

    # Chemins vers les en-têtes (dossiers créés lors de tes compilations)
    INCLUDEPATH += /usr/local/include \
                   /home/gand/openauto/include \
                   /home/gand/aasdk/include

    # Bibliothèques à lier
    LIBS += -L/usr/local/lib -laasdk -laasdk_proto -lrtaudio -lopenauto
    LIBS += -lprotobuf -lusb-1.0 -lssl -lcrypto -lboost_system -lboost_thread

    # Moteur vidéo GStreamer pour l'accélération matérielle
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0
}
