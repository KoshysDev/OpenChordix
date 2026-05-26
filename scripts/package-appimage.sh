#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="${1:-${ROOT_DIR}/build}"
APP_DIR="${2:-${ROOT_DIR}/AppDir}"
LINUXDEPLOY="${LINUXDEPLOY:-${ROOT_DIR}/linuxdeploy.AppImage}"

if [ ! -x "${LINUXDEPLOY}" ]; then
  echo "linuxdeploy AppImage is missing or is not executable: ${LINUXDEPLOY}" >&2
  exit 1
fi

rm -rf "${APP_DIR}"
cmake --install "${BUILD_DIR}" --config Release --prefix "${APP_DIR}/usr"

mkdir -p "${APP_DIR}/usr/share/applications"
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/256x256/apps"
cp "${ROOT_DIR}/assets/icons/AppIcon.png" "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/openchordix.png"
cat > "${APP_DIR}/usr/share/applications/openchordix.desktop" <<'EOF'
[Desktop Entry]
Name=OpenChordix
Comment=OpenChordix Pitch Detection
Exec=OpenChordix
Icon=openchordix
Terminal=false
Type=Application
Categories=AudioVideo;Audio;
EOF

library_path=""
for directory in \
  "${BUILD_DIR}/vcpkg_installed/x64-linux/lib" \
  "${ROOT_DIR}/vcpkg_installed/x64-linux/lib"; do
  if [ -d "${directory}" ]; then
    if [ -z "${library_path}" ]; then
      library_path="${directory}"
    else
      library_path="${library_path}:${directory}"
    fi
  fi
done

if [ -n "${library_path}" ]; then
  export LD_LIBRARY_PATH="${library_path}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
fi

export APPIMAGE_NAME="${APPIMAGE_NAME:-OpenChordix}"
APPIMAGE_EXTRACT_AND_RUN=1 "${LINUXDEPLOY}" \
  --appdir "${APP_DIR}" \
  --executable "${APP_DIR}/usr/bin/OpenChordix" \
  --desktop-file "${APP_DIR}/usr/share/applications/openchordix.desktop" \
  --icon-file "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/openchordix.png" \
  --output appimage
