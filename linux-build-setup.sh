#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$ROOT_DIR"

if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
else
  JOBS=4
fi

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
      nasm yasm \
      alsa-lib libxinerama libxcursor libxrandr libxkbcommon \
      xorg-server-devel mesa
  elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y \
      build-essential cmake git pkg-config \
      nasm yasm \
      libasound2-dev libxinerama-dev libxcursor-dev libxrandr-dev \
      libxkbcommon-dev xorg-dev libglu1-mesa-dev
  fi
fi

if [ ! -d "external/bgfx" ] || [ -z "$(ls -A external/bgfx 2>/dev/null)" ]; then
  git clone --depth 1 https://github.com/bkaradzic/bgfx.git external/bgfx
fi
if [ ! -d "external/bx" ] || [ -z "$(ls -A external/bx 2>/dev/null)" ]; then
  git clone --depth 1 https://github.com/bkaradzic/bx.git external/bx
fi
if [ ! -d "external/bimg" ] || [ -z "$(ls -A external/bimg 2>/dev/null)" ]; then
  git clone --depth 1 https://github.com/bkaradzic/bimg.git external/bimg
fi

if [ ! -d "vcpkg" ] || [ -z "$(ls -A vcpkg 2>/dev/null)" ]; then
  git clone --depth 1 https://github.com/microsoft/vcpkg.git vcpkg
fi

VCPKG_DISABLE_METRICS=1 ./vcpkg/bootstrap-vcpkg.sh
VCPKG_DISABLE_METRICS=1 ./vcpkg/vcpkg install

make -C external/bgfx linux-gcc-release64 -j"$JOBS"

cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j"$JOBS"

echo "Build complete: $ROOT_DIR/build/src/app/OpenChordix"
