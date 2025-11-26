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

*   **Core Language:** C++ (C++17 or later)
*   **Build System:** CMake
*   **Real-time Audio:** RtAudio
*   **Graphics/Windowing:** BGFX
*   **UI:** Dear ImGui
*   **Pitch Detection:** Aubio

## Platform Support 💻

*   🐧 **Linux:** Primary Target
*   ⊞ **Windows:** Working just fine
*   🍎 **macOS:** Contributions welcome (No current development plans)

## Building 🏗️

### Dependencies (Linux - Arch Example)

```bash
# Install essential tools if needed
sudo pacman -Syu base-devel cmake git

# Install RtAudio and AUBIO libraries
sudo pacman -S rtaudio aubio glfw-x11 --needed

# Build bgfx/bx/bimg (used from external/) with the bgfx makefile
# Example (from project root):
# make -C external/bgfx CONFIG=Debug
# make -C external/bgfx CONFIG=Release
# make -C external/bgfx tools
```

### Compilation

```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
cmake -B build -DCMAKE_BUILD_TYPE=Release # Or Debug
cmake --build build
```

## ⚠️ Windows Build Instructions

> **Note:** Windows build is an annoying setup. I recommend Linux for a much smoother and minimal-hassle development setup.

### ✅ Requirements

Make sure you have the following installed on Windows:

- **[MinGW-w64 (with g++ support)](https://www.mingw-w64.org/)**
- **[Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)** (select CMake, MSBuild during installation)
- **CMake ≥ 3.25**
- **Git**

Ensure MinGW's `bin` directory (e.g. `C:\ProgramData\mingw64\mingw64\bin`) is in your system `PATH`.

### 1. Clone the project

```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
```

### 2. Get vcpkg (if not already installed)
If you don't have vcpkg installed, clone it inside the project:
```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg/bootstrap-vcpkg.bat
```

### 4. Build the project
You can build manually:
```bash
mkdir build

cmake -S . -B build -G "MinGW Makefiles" ^
  -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic ^
  -DCMAKE_CXX_COMPILER=C:/ProgramData/mingw64/mingw64/bin/g++.exe

cmake --build build --config Release
```

Or just run the build.bat:
```bash
./build.bat
```
This script will:
- Configure the project with MinGW and vcpkg
- Build it using CMake

## Contributing ❤️

Contributions, bug reports, and feature suggestions are welcome! Please open an issue or submit a pull request.

## License 📜

This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) file for details.
