#!/usr/bin/env bash
set -euo pipefail

# Installe les dépendances système pour compiler et exécuter InterfaceGPS
# Cible principale: Debian/Ubuntu/Raspberry Pi OS

if [[ "${EUID}" -ne 0 ]]; then
  SUDO="sudo"
else
  SUDO=""
fi

command -v apt-get >/dev/null 2>&1 || {
  echo "[ERREUR] Ce script supporte uniquement les distributions basées sur APT (Debian/Ubuntu/Raspberry Pi OS)."
  echo "Installez manuellement les dépendances Qt6 indiquées dans docs/build.md."
  exit 1
}

echo "[INFO] Mise à jour des index APT..."
$SUDO apt-get update

echo "[INFO] Installation des dépendances build/runtime..."
$SUDO apt-get install -y \
  build-essential \
  qt6-base-dev \
  qt6-base-dev-tools \
  qt6-tools-dev \
  qt6-tools-dev-tools \
  qt6-declarative-dev \
  qt6-declarative-dev-tools \
  qt6-positioning-dev \
  qt6-location-dev \
  qt6-serialport-dev \
  qt6-multimedia-dev \
  qt6-connectivity-dev \
  qt6-webengine-dev \
  qml6-module-qtquick \
  qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts \
  qml6-module-qtlocation \
  qml6-module-qtpositioning \
  qml6-module-qtmultimedia \
  qml6-module-qtwebengine \
  libqt6svg6-dev \
  dbus \
  bluez \
  i2c-tools \
  gpsd gpsd-clients \
  xvfb \
  doxygen

echo "[INFO] Vérification qmake6..."
if ! command -v qmake6 >/dev/null 2>&1; then
  echo "[ERREUR] qmake6 introuvable après installation."
  exit 1
fi

echo "[INFO] Dépendances installées avec succès."
echo "[INFO] Vous pouvez maintenant compiler avec:"
echo "       qmake6 InterfaceGPS.pro && make -j\"\$(nproc)\""
