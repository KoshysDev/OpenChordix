cmake_minimum_required(VERSION 3.19.8)
project(OpenChordix VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Dependencies ---
find_package(rtaudio QUIET)
find_package(aubio QUIET)
find_package(Threads REQUIRED)

# Use PkgConfig as a fallback or primary method on non-Windows systems
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(PkgConfig REQUIRED)

    # Find rtaudio via pkg-config if find_package didn't find it
    if(NOT rtaudio_FOUND)
        message(STATUS "rtaudio not found via find_package. Trying pkg-config...")
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
    if(NOT aubio_FOUND)
         message(STATUS "aubio not found via find_package. Trying pkg-config...")
         pkg_check_modules(AUBIO_PC REQUIRED aubio)
         if(NOT AUBIO_PC_FOUND)
            message(FATAL_ERROR "aubio could not be found using find_package or pkg-config.")
         endif()
    endif()

elseif(NOT rtaudio_FOUND OR NOT aubio_FOUND)
    # On Windows, if find_package failed, it's a fatal error (assuming vcpkg)
    message(FATAL_ERROR "rtaudio or aubio package not found via find_package on Windows. Ensure vcpkg setup is correct.")
endif()


# --- Executable ---
add_executable(OpenChordix
    src/main.cpp
    src/PitchDetector.cpp
    src/AudioManager.cpp
    src/NoteConverter.cpp
)

# --- Include Source Directory ---
target_include_directories(OpenChordix PRIVATE src)

# --- Include Directories from Dependencies ---
if(rtaudio_FOUND)
    get_target_property(RTAUDIO_INCLUDE_DIRS RtAudio::rtaudio INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(OpenChordix PRIVATE ${RTAUDIO_INCLUDE_DIRS})
elseif(RTAUDIO_PC_FOUND)
    target_include_directories(OpenChordix PRIVATE ${RTAUDIO_PC_INCLUDE_DIRS})
    if(ALSA_PC_FOUND)
        target_include_directories(OpenChordix PRIVATE ${ALSA_PC_INCLUDE_DIRS})
    endif()
endif()

if(aubio_FOUND)
    get_target_property(AUBIO_INCLUDE_DIRS aubio::aubio INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(OpenChordix PRIVATE ${AUBIO_INCLUDE_DIRS})
elseif(AUBIO_PC_FOUND)
    target_include_directories(OpenChordix PRIVATE ${AUBIO_PC_INCLUDE_DIRS})
endif()


# --- Link Libraries ---
if(rtaudio_FOUND)
    message(STATUS "Linking RtAudio using IMPORTED target RtAudio::rtaudio")
    target_link_libraries(OpenChordix PRIVATE RtAudio::rtaudio)
elseif(RTAUDIO_PC_FOUND)
    message(STATUS "Linking RtAudio using pkg-config variables")
    target_link_libraries(OpenChordix PRIVATE ${RTAUDIO_PC_LIBRARIES})
    if(ALSA_PC_FOUND)
        message(STATUS "Linking ALSA using pkg-config variables")
        target_link_libraries(OpenChordix PRIVATE ${ALSA_PC_LIBRARIES})
    endif()
    message(STATUS "Linking Threads explicitly")
    target_link_libraries(OpenChordix PRIVATE Threads::Threads)
else()
   message(FATAL_ERROR "Cannot link rtaudio - library not found.")
endif()

if(aubio_FOUND)
   message(STATUS "Linking Aubio using IMPORTED target aubio::aubio")
   target_link_libraries(OpenChordix PRIVATE aubio::aubio)
elseif(AUBIO_PC_FOUND)
   message(STATUS "Linking Aubio using pkg-config variables")
   target_link_libraries(OpenChordix PRIVATE ${AUBIO_PC_LIBRARIES})
else()
   message(FATAL_ERROR "Cannot link aubio - library not found.")
endif()

# Link Threads::Threads again just in case it wasn't linked via rtaudio pkg-config section
if(NOT rtaudio_FOUND)
    find_package(Threads REQUIRED) 
    target_link_libraries(OpenChordix PRIVATE Threads::Threads)
endif()


# --- Diagnostic Messages (Comment out for prod) ---
message(STATUS "Project Source Dir: ${PROJECT_SOURCE_DIR}")
if(aubio_FOUND)
    get_target_property(AUBIO_INCLUDE_DIRS aubio::aubio INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(AUBIO_LINK_LIBRARIES aubio::aubio INTERFACE_LINK_LIBRARIES)
    message(STATUS "Found Aubio includes (via target): ${AUBIO_INCLUDE_DIRS}")
    message(STATUS "Found Aubio libraries (via target): ${AUBIO_LINK_LIBRARIES}")
elseif(AUBIO_PC_FOUND)
    message(STATUS "Found Aubio includes (via pkg-config): ${AUBIO_PC_INCLUDE_DIRS}")
    message(STATUS "Found Aubio libraries (via pkg-config): ${AUBIO_PC_LIBRARIES}")
endif()

if(rtaudio_FOUND)
    get_target_property(RTAUDIO_INCLUDE_DIRS RtAudio::rtaudio INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(RTAUDIO_LINK_LIBRARIES RtAudio::rtaudio INTERFACE_LINK_LIBRARIES)
    message(STATUS "Found RtAudio includes (via target): ${RTAUDIO_INCLUDE_DIRS}")
    message(STATUS "Found RtAudio link libraries (via target): ${RTAUDIO_LINK_LIBRARIES}")
elseif(RTAUDIO_PC_FOUND)
   message(STATUS "Found RtAudio includes (via pkg-config): ${RTAUDIO_PC_INCLUDE_DIRS}")
   message(STATUS "Found RtAudio libraries (via pkg-config): ${RTAUDIO_PC_LIBRARIES}")
   if(ALSA_PC_FOUND)
       message(STATUS "Found ALSA includes (via pkg-config): ${ALSA_PC_INCLUDE_DIRS}")
       message(STATUS "Found ALSA libraries (via pkg-config): ${ALSA_PC_LIBRARIES}")
   endif()
endif()