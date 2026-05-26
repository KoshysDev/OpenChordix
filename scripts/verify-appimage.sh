#!/usr/bin/env sh
set -eu

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 path/to/OpenChordix.AppImage" >&2
  exit 1
fi

APPIMAGE="$(CDPATH= cd -- "$(dirname -- "$1")" && pwd)/$(basename -- "$1")"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

cd "${WORK_DIR}"
"${APPIMAGE}" --appimage-extract >/dev/null

APP_ROOT="${WORK_DIR}/squashfs-root"
EXECUTABLE="${APP_ROOT}/usr/bin/OpenChordix"
APP_RUN="${APP_ROOT}/AppRun"
if [ ! -x "${EXECUTABLE}" ] || [ ! -x "${APP_RUN}" ]; then
  echo "Extracted AppImage is missing AppRun or usr/bin/OpenChordix." >&2
  exit 1
fi

runtime_library_path="${APP_ROOT}/usr/lib"
if [ -d "${APP_ROOT}/usr/lib/x86_64-linux-gnu" ]; then
  runtime_library_path="${runtime_library_path}:${APP_ROOT}/usr/lib/x86_64-linux-gnu"
fi

check_dependencies() {
  target="$1"
  output="$(LD_LIBRARY_PATH="${runtime_library_path}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" ldd "${target}")"
  printf '%s\n' "${output}"
  if printf '%s\n' "${output}" | grep -q 'not found'; then
    echo "Missing shared-library dependency in AppImage for ${target}." >&2
    exit 1
  fi
}

check_dependencies "${EXECUTABLE}"
check_dependencies "${APP_RUN}"

APPIMAGE_EXTRACT_AND_RUN=1 "${APPIMAGE}" --version
echo "AppImage validation passed."
