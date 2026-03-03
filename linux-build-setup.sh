#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$ROOT_DIR"

if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
else
  JOBS=4
fi

VCPKG_BINARY_CACHE_DIR="${ROOT_DIR}/external/vcpkg/binary_cache"
mkdir -p "${VCPKG_BINARY_CACHE_DIR}"
export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_BINARY_CACHE_DIR}"
export VCPKG_BINARY_SOURCES="clear;files,${VCPKG_BINARY_CACHE_DIR},readwrite"
export VCPKG_ROOT="${ROOT_DIR}/external/vcpkg"
export VCPKG_DISABLE_METRICS=1

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    return 1
  fi
}

need_cmd git
need_cmd cmake
need_cmd make

if command -v sudo >/dev/null 2>&1; then
  if command -v pacman >/dev/null 2>&1; then
    sudo pacman -Syu --needed \
      base-devel cmake git pkgconf \
      autoconf autoconf-archive automake libtool \
      nasm yasm \
      alsa-lib libpulse libxinerama libxcursor libxrandr libxkbcommon \
      xorg-server-devel mesa
  elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y \
      build-essential cmake git pkg-config \
      autoconf autoconf-archive automake libtool libltdl-dev \
      nasm yasm \
      libasound2-dev libpulse-dev \
      libxinerama-dev libxcursor-dev libxrandr-dev \
      libxkbcommon-dev xorg-dev libglu1-mesa-dev
  fi
fi

if [ -f ".gitmodules" ]; then
  submodules="external/bgfx external/bx external/bimg"
  if [ -d "external/vcpkg" ] && [ -n "$(ls -A external/vcpkg 2>/dev/null)" ]; then
    if [ -x "external/vcpkg/bootstrap-vcpkg.sh" ]; then
      echo "external/vcpkg already exists; skipping submodule init for vcpkg."
    else
      backup="external/vcpkg.invalid-$(date +%Y%m%d%H%M%S)"
      echo "external/vcpkg exists but is missing bootstrap-vcpkg.sh; moving to $backup."
      mv external/vcpkg "$backup"
      submodules="$submodules external/vcpkg"
    fi
  else
    submodules="$submodules external/vcpkg"
  fi
  git submodule update --init --recursive $submodules
else
  if [ ! -d "external/bgfx" ] || [ -z "$(ls -A external/bgfx 2>/dev/null)" ]; then
    git clone --depth 1 https://github.com/bkaradzic/bgfx.git external/bgfx
  fi
  if [ ! -d "external/bx" ] || [ -z "$(ls -A external/bx 2>/dev/null)" ]; then
    git clone --depth 1 https://github.com/bkaradzic/bx.git external/bx
  fi
  if [ ! -d "external/bimg" ] || [ -z "$(ls -A external/bimg 2>/dev/null)" ]; then
    git clone --depth 1 https://github.com/bkaradzic/bimg.git external/bimg
  fi
  if [ -d "external/vcpkg" ] && [ -n "$(ls -A external/vcpkg 2>/dev/null)" ] && [ ! -x "external/vcpkg/bootstrap-vcpkg.sh" ]; then
    backup="external/vcpkg.invalid-$(date +%Y%m%d%H%M%S)"
    echo "external/vcpkg exists but is missing bootstrap-vcpkg.sh; moving to $backup."
    mv external/vcpkg "$backup"
  fi
  if [ ! -d "external/vcpkg" ] || [ -z "$(ls -A external/vcpkg 2>/dev/null)" ]; then
    git clone --depth 1 https://github.com/microsoft/vcpkg.git external/vcpkg
  fi
fi

./external/vcpkg/bootstrap-vcpkg.sh
./external/vcpkg/vcpkg install --vcpkg-root="$VCPKG_ROOT"

BGFX_BUILD_DIR="external/bgfx/.build"
BGFX_STAMP_FILE="${BGFX_BUILD_DIR}/.openchordix-bgfx-head"
BGFX_HEAD="$(git -C external/bgfx rev-parse HEAD)"

# Regenerate bgfx build files when source revision changed (or when stamp is absent).
if [ -d "${BGFX_BUILD_DIR}" ]; then
  if [ ! -f "${BGFX_STAMP_FILE}" ] || [ "$(cat "${BGFX_STAMP_FILE}")" != "${BGFX_HEAD}" ]; then
    echo "bgfx revision changed or stamp missing; cleaning ${BGFX_BUILD_DIR}."
    rm -rf "${BGFX_BUILD_DIR}"
  fi
fi

if ! make -C external/bgfx linux-gcc-release64 -j"$JOBS"; then
  echo "bgfx build failed; retrying once after clean rebuild."
  rm -rf "${BGFX_BUILD_DIR}"
  make -C external/bgfx linux-gcc-release64 -j"$JOBS"
fi

mkdir -p "${BGFX_BUILD_DIR}"
printf '%s\n' "${BGFX_HEAD}" > "${BGFX_STAMP_FILE}"

cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/external/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j"$JOBS"

echo "Build complete: $ROOT_DIR/build/src/app/OpenChordix"
