cmake_minimum_required(VERSION 3.19.8)
project(OpenChordix VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(OPENCHORDIX_ENABLE_WAYLAND "Enable GLFW Wayland native handles" OFF)
option(OPENCHORDIX_BUILD_TESTS "Build OpenChordix unit tests" ON)
option(OPENCHORDIX_BUILD_RENDERER "Build OpenChordix renderer" ON)
option(OPENCHORDIX_BUILD_APP "Build OpenChordix app" ON)

message(STATUS "Top-Level: Finding External Dependencies...")

# --- Find EXTERNAL Dependencies Here ---
find_package(RtAudio QUIET)
find_package(Aubio QUIET)
find_package(Threads REQUIRED)

# Use PkgConfig as a fallback or primary method on non-Windows systems
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(PkgConfig REQUIRED)

    # Find rtaudio via pkg-config if find_package didn't find it
    if(NOT RtAudio_FOUND)
        message(STATUS "Top-Level: RtAudio not found via find_package. Trying pkg-config...")
        pkg_check_modules(RTAUDIO_PC REQUIRED rtaudio)
        if(NOT RTAUDIO_PC_FOUND)
            message(FATAL_ERROR "rtaudio could not be found using find_package or pkg-config.")
        endif()
        pkg_check_modules(ALSA_PC REQUIRED alsa)
        if(NOT ALSA_PC_FOUND)
            message(WARNING "ALSA pkg-config not found, rtaudio might have issues.")
        endif()
    endif()

    # Find aubio via pkg-config if find_package didn't find it
    if(NOT Aubio_FOUND AND NOT TARGET Aubio::aubio)
        message(STATUS "Top-Level: Aubio not found via find_package. Trying pkg-config...")
        pkg_check_modules(AUBIO_PC REQUIRED aubio)
        if(NOT AUBIO_PC_FOUND)
            message(FATAL_ERROR "aubio could not be found using find_package or pkg-config.")
        endif()
    endif()

elseif(NOT RtAudio_FOUND OR NOT Aubio_FOUND)
    # On Windows, if either package failed find_package (QUIET), it's an error
    message(FATAL_ERROR "rtaudio or aubio package not found via find_package on Windows. Ensure vcpkg setup is correct.")
endif()


message(STATUS "Top-Level: Adding subdirectories...")

# --- subdirectories ---
add_subdirectory(src/core)
add_subdirectory(src/devtools)
if(OPENCHORDIX_BUILD_RENDERER)
    add_subdirectory(src/graphics)
endif()
if(OPENCHORDIX_BUILD_APP)
    if(NOT OPENCHORDIX_BUILD_RENDERER)
        message(FATAL_ERROR "OPENCHORDIX_BUILD_APP requires OPENCHORDIX_BUILD_RENDERER=ON")
    endif()
    add_subdirectory(src/app)
endif()

if(OPENCHORDIX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "Top-Level: Finished processing.")

# --- Optional: Diagnostic Messages ---
message(STATUS "Top-Level: Project Source Dir: ${PROJECT_SOURCE_DIR}")

# Aubio Diagnostics
if(TARGET Aubio::aubio)
    get_target_property(AUBIO_INCLUDE_DIRS Aubio::aubio INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(AUBIO_LINK_LIBRARIES Aubio::aubio INTERFACE_LINK_LIBRARIES)
    message(STATUS "Top-Level: Found Aubio includes (via target): ${AUBIO_INCLUDE_DIRS}")
    message(STATUS "Top-Level: Found Aubio libraries (via target): ${AUBIO_LINK_LIBRARIES}")
elseif(AUBIO_PC_FOUND)
    message(STATUS "Top-Level: Found Aubio includes (via pkg-config): ${AUBIO_PC_INCLUDE_DIRS}")
    message(STATUS "Top-Level: Found Aubio libraries (via pkg-config): ${AUBIO_PC_LIBRARIES}")
elseif(Aubio_FOUND)
    message(STATUS "Top-Level: Found Aubio via find_package (no target): Includes='${Aubio_INCLUDE_DIRS}', Libs='${Aubio_LIBRARIES}'")
else()
    message(WARNING "Top-Level: Aubio not found.")
endif()

# RtAudio Diagnostics
if(TARGET RtAudio::rtaudio)
    get_target_property(RTAUDIO_INCLUDE_DIRS RtAudio::rtaudio INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(RTAUDIO_LINK_LIBRARIES RtAudio::rtaudio INTERFACE_LINK_LIBRARIES)
    message(STATUS "Top-Level: Found RtAudio includes (via target): ${RTAUDIO_INCLUDE_DIRS}")
    message(STATUS "Top-Level: Found RtAudio link libraries (via target): ${RTAUDIO_LINK_LIBRARIES}")
elseif(RTAUDIO_PC_FOUND)
    message(STATUS "Top-Level: Found RtAudio includes (via pkg-config): ${RTAUDIO_PC_INCLUDE_DIRS}")
    message(STATUS "Top-Level: Found RtAudio libraries (via pkg-config): ${RTAUDIO_PC_LIBRARIES}")
    if(ALSA_PC_FOUND)
        message(STATUS "Top-Level: Found ALSA includes (via pkg-config): ${ALSA_PC_INCLUDE_DIRS}")
        message(STATUS "Top-Level: Found ALSA libraries (via pkg-config): ${ALSA_PC_LIBRARIES}")
    endif()
elseif(RtAudio_FOUND)
    message(STATUS "Top-Level: Found RtAudio via find_package (no target): Includes='${RtAudio_INCLUDE_DIRS}', Libs='${RtAudio_LIBRARIES}'")
else()
    message(WARNING "Top-Level: RtAudio not found.")
endif()
