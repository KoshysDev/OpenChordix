# OpenChordix

[![GitHub repo size](https://img.shields.io/github/repo-size/KoshysDev/OpenChordix)](https://github.com/KoshysDev/OpenChordix/)
[![GitHub last commit](https://img.shields.io/github/last-commit/KoshysDev/OpenChordix)](https://github.com/KoshysDev/OpenChordix/commits/linux-build)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

**An open-source, cross-platform game/tool inspired by Rocksmith for learning guitar and bass using real-time pitch detection. Without sound detection setting up pain.**

## Current Status (🌱 Early Development)

*   ✅ Core audio input/output using RtAudio.
*   ✅ Runtime selection of Audio API (ALSA, Pulse, JACK, Auto).
*   ✅ Audio device listing and input device selection.
*   ✅ Basic audio monitoring loop (input -> output).
*   ✅ Note and Pitch detection.

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
*   ⊞ **Windows:** Secondary Target (Planned for later)
*   🍎 **macOS:** Contributions welcome (No current development plans)

## Building 🏗️

### Dependencies (Linux - Arch Example)

```bash
# Install essential tools if needed
sudo pacman -Syu base-devel cmake git
#Or if you have yay:
yay -Syu base-devel cmake git

# Install RtAudio and ALSA libraries
sudo pacman -S rtaudio alsa-lib --needed
# Or:
yay -S rtaudio alsa-lib --needed
```

### Compilation

```bash
git clone https://github.com/KoshysDev/OpenChordix.git
cd OpenChordix
cmake -B build -DCMAKE_BUILD_TYPE=Release # Or Debug
cmake --build build
```

## Contributing ❤️

Contributions, bug reports, and feature suggestions are welcome! Please open an issue or submit a pull request.

## License 📜

This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) file for details.
