set -euo pipefail

# --- Detect Arch ---
if ! command -v pacman >/dev/null 2>&1; then
  echo "[ERROR] This script targets Arch/Manjaro (pacman)."
  echo "Install deps manually for your distro, then run the cmake lines below."
  exit 1
fi

# --- Install deps (Arch packages) ---
sudo pacman -Syu --needed \
  base-devel cmake git pkgconf ninja \
  sdl2 bgfx imgui nlohmann-json \
  rtaudio aubio ffmpeg

# --- Configure + Build ---
GEN="Ninja"
if ! command -v ninja >/dev/null 2>&1; then
  GEN="Unix Makefiles"
fi

BUILD_DIR="build"
rm -rf "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR" -G "$GEN" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j
