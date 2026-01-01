# OpenChordix

[![GitHub repo size](https://img.shields.io/github/repo-size/KoshysDev/OpenChordix)](https://github.com/KoshysDev/OpenChordix/)
[![GitHub last commit (linux-build)](https://img.shields.io/github/last-commit/KoshysDev/OpenChordix/bgfx)](https://github.com/KoshysDev/OpenChordix/commits/bgfx)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

**Branch: `bgfx` (✅ Active Development Branch)**

**An open-source, cross-platform game/tool inspired by Rocksmith for learning guitar and bass using real-time pitch detection. Without sound detection setting up pain.**

## Current Status (🌱 Early Development)

*   ✅ Core audio input/output using RtAudio.
*   ✅ Runtime selection of Audio API (ALSA, Pulse, JACK, Auto).
*   ✅ Note and Pitch detection.
*   🛠️ Working on: GUI

## Key Features Roadmap 🚀

The project aims to implement the following major features incrementally:

*   🔊 **Core Audio Processing:** Robust real-time audio handling & low latency.
*   🎧 **Pitch Detection:** Accurate fundamental frequency detection.
*   🖥️ **Graphical Interface:** Windowing, menus, settings UI.
*   🎸 **Visual Tuner:** Clear and responsive tuning mode.
*   🎼 **Note Highway & Gameplay:** Scrolling notes, hit detection, scoring.
*   📃 **Song Format & Management:** Loading songs (potential Guitar Pro import).
*   🛠 **Training Tools:** Speed control, section looping.
*   🎶 **(Stretch) Tone Designer:** Advanced amp/effect simulation.
*   💻 **Cross-Platform:** Windows compatibility.

## Technology Stack 🛠️

*   **Core Language:** C++20
*   **Build System:** CMake (>= 3.19.8)
*   **Dependency Manager:** vcpkg
*   **Real-time Audio:** RtAudio (6.x)
*   **Pitch Detection:** Aubio (0.4.x)
*   **Graphics/Windowing:** BGFX (1.129.x) + GLFW (3.4)
*   **UI:** Dear ImGui
*   **Image Loading:** stb

## Platform Support 💻

*   🐧 **Linux:** Primary Target
*   ⊞ **Windows:** In progress
*   🍎 **macOS:** Contributions welcome (No current development plans)

## Building 🏗️

### Linux setup + build

```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
chmod +x linux-build-setup.sh
./linux-build-setup.sh
```

### Linux manual setup + build

#### 1) Install system packages

Arch (example):
```bash
sudo pacman -Syu --needed base-devel cmake git pkgconf \
  alsa-lib libxinerama libxcursor libxrandr libxkbcommon \
  xorg-server-devel mesa
```

Ubuntu/Debian (example):
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config \
  libasound2-dev libxinerama-dev libxcursor-dev libxrandr-dev \
  libxkbcommon-dev xorg-dev libglu1-mesa-dev
```

#### 2) Clone the project
```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
```

#### 3) Fetch bgfx/bx/bimg sources
```bash
git clone --depth 1 https://github.com/bkaradzic/bgfx.git external/bgfx
git clone --depth 1 https://github.com/bkaradzic/bx.git external/bx
git clone --depth 1 https://github.com/bkaradzic/bimg.git external/bimg
```

#### 4) Bootstrap vcpkg and install deps
```bash
git clone --depth 1 https://github.com/microsoft/vcpkg.git vcpkg
VCPKG_DISABLE_METRICS=1 ./vcpkg/bootstrap-vcpkg.sh
VCPKG_DISABLE_METRICS=1 ./vcpkg/vcpkg install
```

#### 5) Build bgfx (Release)
```bash
make -C external/bgfx linux-gcc-release64 -j$(nproc)
```

#### 6) Configure + build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j$(nproc)
```

#### Output
```bash
./build/src/app/OpenChordix
```

### Windows

Windows setup is in progress. If you want to help test or document it, open an issue.

## Contributing ❤️

Contributions, bug reports, and feature suggestions are welcome! Please open an issue or submit a pull request.

## Known Issues / Bugs

- **BGFX on Hyprland (Wayland)**  
  BGFX may fail to start a native window session under Hyprland.

  **Workaround:** build with Wayland support enabled:

  ```sh
  cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DOPENCHORDIX_ENABLE_WAYLAND=ON \
    -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake

  cmake --build build -j$(nproc)

## License 📜

This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) file for details.
