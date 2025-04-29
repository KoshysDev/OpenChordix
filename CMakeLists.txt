cmake_minimum_required(VERSION 3.19.8)
project(OpenChordix VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "Top-Level: Finding External Dependencies...")

# --- Find EXTERNAL Dependencies Here ---
find_package(rtaudio QUIET)
find_package(aubio QUIET)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)     
find_package(bgfx QUIET)  

# Use PkgConfig as a fallback or primary method on non-Windows systems
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(PkgConfig REQUIRED)

    # Find rtaudio via pkg-config if find_package didn't find it
    if(NOT rtaudio_FOUND)
        message(STATUS "Top-Level: rtaudio not found via find_package. Trying pkg-config...")
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
    if(NOT aubio_FOUND AND NOT TARGET aubio::aubio)
         message(STATUS "Top-Level: aubio not found via find_package. Trying pkg-config...")
         pkg_check_modules(AUBIO_PC REQUIRED aubio)
         if(NOT AUBIO_PC_FOUND)
            message(FATAL_ERROR "aubio could not be found using find_package or pkg-config.")
         endif()
    endif()

    # Find bgfx
    if(NOT bgfx_FOUND AND NOT TARGET bgfx::bgfx)
         message(STATUS "Top-Level: bgfx not found via find_package. Trying pkg-config...")
         pkg_check_modules(BGFX_PC REQUIRED bgfx)
    endif()

    # Find SDL2
    if(NOT SDL2_FOUND AND NOT TARGET SDL2::SDL2)
         message(STATUS "Top-Level: SDL2 target not found via find_package. Trying pkg-config...")
         pkg_check_modules(SDL2_PC REQUIRED sdl2)
    endif()

elseif(NOT rtaudio_FOUND OR NOT aubio_FOUND OR NOT bgfx_FOUND OR NOT SDL2_FOUND)
    # On Windows, if either package failed find_package (QUIET), it's an error
    message(FATAL_ERROR "rtaudio or aubio package not found via find_package on Windows. Ensure vcpkg setup is correct.")
endif()


message(STATUS "Top-Level: Adding subdirectories...")

# --- ubdirectories ---
add_subdirectory(src/core)
#add_subdirectory(src/gui)
add_subdirectory(src/app)

message(STATUS "Top-Level: Finished processing.")

# --- Optional: Diagnostic Messages ---
message(STATUS "Top-Level: Project Source Dir: ${PROJECT_SOURCE_DIR}")

# Aubio Diagnostics
if(TARGET aubio::aubio)
    get_target_property(AUBIO_INCLUDE_DIRS aubio::aubio INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(AUBIO_LINK_LIBRARIES aubio::aubio INTERFACE_LINK_LIBRARIES)
    message(STATUS "Top-Level: Found Aubio includes (via target): ${AUBIO_INCLUDE_DIRS}")
    message(STATUS "Top-Level: Found Aubio libraries (via target): ${AUBIO_LINK_LIBRARIES}")
elseif(AUBIO_PC_FOUND)
    message(STATUS "Top-Level: Found Aubio includes (via pkg-config): ${AUBIO_PC_INCLUDE_DIRS}")
    message(STATUS "Top-Level: Found Aubio libraries (via pkg-config): ${AUBIO_PC_LIBRARIES}")
elseif(aubio_FOUND)
    message(STATUS "Top-Level: Found Aubio via find_package (no target): Includes='${aubio_INCLUDE_DIRS}', Libs='${aubio_LIBRARIES}'")
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
elseif(rtaudio_FOUND)
   message(STATUS "Top-Level: Found RtAudio via find_package (no target): Includes='${rtaudio_INCLUDE_DIRS}', Libs='${rtaudio_LIBRARIES}'")
else()
    message(WARNING "Top-Level: RtAudio not found.")
endif()