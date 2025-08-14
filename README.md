# OpenChordix

[![GitHub repo size](https://img.shields.io/github/repo-size/KoshysDev/OpenChordix)](https://github.com/KoshysDev/OpenChordix/)
[![GitHub last commit (linux-build)](https://img.shields.io/github/last-commit/KoshysDev/OpenChordix/feature/gui)](https://github.com/KoshysDev/OpenChordix/commits/feature/gui)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

**Branch: `feature/gui` (✅ Active Development Branch)**

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
sudo pacman -S rtaudio aubio --needed
```

### Compilation
```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Windows Build Instructions

> TL;DR: run: .\build-windows-msvc.bat

### ✅ Requirements

- **[MinGW-w64 (with g++ support)](https://www.mingw-w64.org/)**
- **[Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)** (select CMake, MSBuild during installation)
- **CMake ≥ 3.10**
- **Git**
- **Ninja (auto-installed by the script via winget if missing)**

### One-shot build
```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
.\build-windows-msvc.bat
```

The script will:
- Ensure Ninja is installed (via winget).
- Clone & bootstrap vcpkg into external\vcpkg (if missing).
- Enter the VS 2022 dev environment.
- Configure with Ninja + MSVC and the vcpkg toolchain.
- Build build-ninja\src\app\OpenChordix.exe.

### Manual build (alternative)
Open Developer PowerShell for VS 2022 (or call vcvarsall.bat amd64), then:

```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
```

```bash
$env:VCPKG_ROOT = "$PWD\external\vcpkg"
if (!(Test-Path $env:VCPKG_ROOT)) {
  git clone https://github.com/microsoft/vcpkg.git external/vcpkg
  & "$env:VCPKG_ROOT\bootstrap-vcpkg.bat"
}
```

```bash
$ninja = (Get-Command ninja).Source
cmake -G Ninja -S . -B build-ninja `
  -D CMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -D VCPKG_TARGET_TRIPLET=x64-windows `
  -D CMAKE_MAKE_PROGRAM="$ninja"
cmake --build build-ninja
```

#### MSVC flags note (bgfx/bx)
CMake will inject these automatically via top-level CMake, but if you tweak flags, keep:
- /Zc:__cplusplus and /Zc:preprocessor
- Consider /permissive- and /bigobj for large translation units.

---

## Optional: Windows (MinGW community triplet)

MinGW works but is more fragile. If you need it:

- Install MSYS2 with MinGW-w64 and make sure C:\msys64\mingw64\bin is first on PATH in a non-VS shell.
- Use the provided CMakePresets.json preset mingw-vcpkg (community triplet x64-mingw-dynamic).

```bash
$env:PATH = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;$env:PATH"
cmake --preset mingw-vcpkg
cmake --build --preset mingw-vcpkg-release
```

---

## Troubleshooting

- CMake can’t find compiler on Windows  
  Use a VS dev shell or run vcvarsall.bat amd64.

- bgfx/bx errors about MSVC flags  
  Ensure /Zc:__cplusplus and /Zc:preprocessor are present (the project sets them).

- vcpkg ports failing with LOCATION policy error (yasm)  
  Use newer vcpkg or (temporary) add  
  -D CMAKE_POLICY_DEFAULT_CMP0026=OLD to VCPKG_CMAKE_CONFIGURE_OPTIONS.

---

## Contributing ❤️

Contributions, bug reports, and feature suggestions are welcome! Please open an issue or submit a pull request.

---

## License 📜

This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) file for details.