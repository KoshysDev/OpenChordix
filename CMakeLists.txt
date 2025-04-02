cmake_minimum_required(VERSION 3.10)

project(openchordix VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find PThreads (Required by RtAudio on Linux/macOS) ---
find_package(Threads REQUIRED)

# --- Find RtAudio using pkg-config (Reliable on Linux) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTAUDIO REQUIRED rtaudio) # Checks for rtaudio.pc

if(NOT RTAUDIO_FOUND)
    # This shouldn't happen if yay -S rtaudio worked, but check just in case
    message(FATAL_ERROR "RtAudio not found using pkg-config. Is rtaudio installed correctly?")
endif()

include_directories(src)

# --- Executable ---
add_executable(openchordix 
    src/main.cpp
    src/AudioManager.cpp)

# --- Include Directories ---
target_include_directories(openchordix PRIVATE
    ${RTAUDIO_INCLUDE_DIRS}  # Add RtAudio include path from pkg-config
    src
)

# --- Link Libraries ---
target_link_libraries(openchordix PRIVATE
    Threads::Threads
    ${RTAUDIO_LIBRARIES}
)

# --- Compile Definitions (For RtAudio Backends) ---
target_compile_definitions(openchordix PRIVATE __LINUX_ALSA__)

# --- Optional: Show found variables for debugging ---
message(STATUS "Project Source Dir: ${PROJECT_SOURCE_DIR}")
message(STATUS "Found RtAudio includes: ${RTAUDIO_INCLUDE_DIRS}")
message(STATUS "Found RtAudio libraries: ${RTAUDIO_LIBRARIES}")
message(STATUS "Found Threads libraries: ${Threads_LIBRARIES}")