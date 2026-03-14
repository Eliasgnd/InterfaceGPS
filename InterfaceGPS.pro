# -------------------------------------------------------------------------
# Section 1 : Modules Qt necessaires
# -------------------------------------------------------------------------
# positioning/location : Pour le parsing GPS et les cartes
# serialport : Pour la communication avec le module u-blox
# quick/qml : Pour l'interface graphique moderne
# multimedia : Pour la lecture audio (MediaPlayer)
# webenginewidgets : Pour l'affichage de composants web
QT += core gui widgets positioning location quickwidgets qml quick \
      serialport virtualkeyboard multimedia quickcontrols2 network \
      dbus bluetooth webenginewidgets testlib svg

# Compatibilite Qt 5/6 pour le module widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# -------------------------------------------------------------------------
# Section 2 : Configuration du compilateur
# -------------------------------------------------------------------------
CONFIG += c++17

# Nom de l'executable genere
TARGET = InterfaceGPS
TEMPLATE = app

# -------------------------------------------------------------------------
# Section 3 : Fichiers sources et En-tetes (C++)
# -------------------------------------------------------------------------
SOURCES += \
    bluetoothmanager.cpp \
    camerapage.cpp \
    clavier.cpp \
    gpstelemetrysource.cpp \
    homeassistant.cpp \
    main.cpp \
    mainwindow.cpp \
    mediapage.cpp \
    mpu9250source.cpp \
    navigationpage.cpp \
    settingspage.cpp \
    telemetrydata.cpp

HEADERS += \
    bluetoothmanager.h \
    camerapage.h \
    clavier.h \
    gpstelemetrysource.h \
    homeassistant.h \
    mainwindow.h \
    mediapage.h \
    mpu9250source.h \
    navigationpage.h \
    settingspage.h \
    telemetrydata.h

# -------------------------------------------------------------------------
# Section 4 : Fichiers d'interface (UI Designer)
# -------------------------------------------------------------------------
FORMS += \
    camerapage.ui \
    mainwindow.ui \
    mediapage.ui \
    navigationpage.ui \
    settingspage.ui

# -------------------------------------------------------------------------
# Section 5 : Ressources (QML, Images, Icones)
# -------------------------------------------------------------------------
# DISTFILES liste les fichiers qui ne sont pas compiles mais font partie du projet
DISTFILES += \
    MediaPlayer.qml \
    diagramme_classe.qmodel \
    map.qml

# Fichier de ressources Qt compile
RESOURCES += \
    resources.qrc
    
# -------------------------------------------------------------------------
# Section 6 : Regles de deploiement (Linux/Unix)
# -------------------------------------------------------------------------
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin

# Ajoute egalement le dossier scripts a la regle d'installation systeme
!isEmpty(target.path): INSTALLS += target
script_install.path = $$target.path/scripts
script_install.files = scripts/*
INSTALLS += script_install
