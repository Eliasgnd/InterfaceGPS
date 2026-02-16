#include "androidautopage.h"

#ifdef Q_OS_LINUX_PI
#include <aasdk/USB/USBHub.hpp> // Exemple d'include aasdk
#endif

AndroidAutoPage::AndroidAutoPage(QWidget* parent) : QWidget(parent) {
    // UI de base pour la page
}

void AndroidAutoPage::startEmulation() {
#ifdef Q_OS_LINUX_PI
    // Ici, vous lancerez la logique de connexion USB avec le téléphone
#endif
}
