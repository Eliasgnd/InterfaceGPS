# On remplace 'quicktest' par les modules de base
QT += qml quick location positioning testlib

# 'qmltestcase' est souvent lié au module 'quicktest' manquant.
# On utilise une configuration plus standard pour Qt 6.
CONFIG += c++17 testcase
TEMPLATE = app

TARGET = qml_map_test

# Cette ligne permet d'utiliser le moteur de test QML sans le module 'quicktest'
CONFIG += qmltestrunner

RESOURCES += \
    ../../resources.qrc \
    qml_map_test.qrc

# Sources pour le lanceur de test C++ si nécessaire
# (Si vous utilisez un main.cpp qui appelle QUICK_TEST_MAIN)
