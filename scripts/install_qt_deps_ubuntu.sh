#!/usr/bin/env bash
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
  echo "Ce script cible Debian/Ubuntu (apt-get introuvable)."
  exit 1
fi

echo "[1/3] Mise à jour index APT..."
sudo apt-get update

choose_pkg() {
  for pkg in "$@"; do
    if apt-cache show "$pkg" >/dev/null 2>&1; then
      echo "$pkg"
      return 0
    fi
  done
  return 1
}

add_pkg() {
  local mode="$1"
  shift
  local chosen
  if chosen="$(choose_pkg "$@")"; then
    PKGS+=("$chosen")
    return 0
  fi

  if [ "$mode" = "required" ]; then
    echo "❌ Aucun paquet disponible parmi (obligatoire): $*"
    exit 1
  else
    echo "⚠️  Aucun paquet disponible parmi (optionnel): $*"
    MISSING_OPTIONAL+=("$*")
  fi
}

PKGS=(
  build-essential
  pkg-config
  cmake
  ninja-build
  dbus
  bluez
  bluetooth
  i2c-tools
  libgl1-mesa-dev
  libxkbcommon-x11-0
  libxcb-cursor0
  xvfb
)
MISSING_OPTIONAL=()

# Outils Qt6
add_pkg required qmake6 qt6-qmake
add_pkg required qt6-base-dev
add_pkg required qt6-base-dev-tools
add_pkg required qt6-tools-dev-tools

# Modules Qt6 utilisés dans InterfaceGPS.pro
add_pkg optional qt6-declarative-dev
add_pkg optional qt6-location-dev
add_pkg optional qt6-positioning-dev
add_pkg optional qt6-serialport-dev
add_pkg optional qt6-multimedia-dev
add_pkg optional qt6-connectivity-dev
add_pkg optional qt6-webengine-dev
add_pkg optional qt6-virtualkeyboard-dev

# Runtimes QML Qt6 (noms variant selon distro)
add_pkg optional qml6-module-qtquick qml-module-qtquick2
add_pkg optional qml6-module-qtquick-controls qml-module-qtquick-controls
add_pkg optional qml6-module-qtquick-layouts qml-module-qtquick-layouts
add_pkg optional qml6-module-qtquick-window qml-module-qtquick-window2
add_pkg optional qml6-module-qt5compat-graphicaleffects qml-module-qtgraphicaleffects
add_pkg optional qml6-module-qtquick-shapes qml-module-qtquick-shapes
add_pkg optional qml6-module-qtlocation qml-module-qtlocation
add_pkg optional qml6-module-qtpositioning qml-module-qtpositioning
add_pkg optional qml6-module-qtwebengine qml-module-qtwebengine
add_pkg optional qml6-module-qtmultimedia qml-module-qtmultimedia

echo "[2/3] Installation toolchain + Qt6 + dépendances runtime..."
sudo apt-get install -y "${PKGS[@]}"

echo "[3/3] Vérification rapide des outils..."
qmake6 -v || true
qt-cmake --version || true

if [ "${#MISSING_OPTIONAL[@]}" -gt 0 ]; then
  echo
  echo "⚠️  Certains paquets optionnels n'ont pas été trouvés sur cette distribution :"
  printf '  - %s\n' "${MISSING_OPTIONAL[@]}"
  echo "La compilation des modules/tests concernés peut échouer si ces dépendances sont réellement requises."
fi

echo "Installation Qt6 terminée ✅"
