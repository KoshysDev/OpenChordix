cmake_minimum_required(VERSION 3.19)
project(OpenChordix VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Dependencies ---
find_package(rtaudio QUIET)

if(UNIX AND NOT rtaudio_FOUND AND NOT CMAKE_TOOLCHAIN_FILE)
  message(STATUS "rtaudio not found via find_package. Trying pkg-config...")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(RTAUDIO_PC REQUIRED rtaudio)
  pkg_check_modules(ALSA_PC alsa)
  find_package(Threads REQUIRED)

  # Check if pkg-config actually found rtaudio
  if(NOT RTAUDIO_PC_FOUND)
    message(FATAL_ERROR "rtaudio could not be found using pkg-config either.")
  endif()

elseif(NOT rtaudio_FOUND)
  message(FATAL_ERROR "rtaudio package not found. Ensure it's installed (e.g., via vcpkg or system package manager) and CMake can find it.")
endif()


# --- Executable ---
add_executable(OpenChordix
    src/main.cpp
    src/AudioManager.cpp
)


# --- Link Libraries ---
if(rtaudio_FOUND)
  message(STATUS "Linking using IMPORTED target rtaudio::rtaudio")
  target_link_libraries(OpenChordix PRIVATE rtaudio::rtaudio)
elseif(RTAUDIO_PC_FOUND)
  # Fallback method: Use variables from pkg-config if that succeeded
  message(STATUS "Linking using pkg-config variables for rtaudio")
  target_include_directories(OpenChordix PRIVATE ${RTAUDIO_PC_INCLUDE_DIRS})
  target_link_libraries(OpenChordix PRIVATE ${RTAUDIO_PC_LIBRARIES})
  if(ALSA_PC_FOUND) # Link ALSA if found via pkg-config too
     target_include_directories(OpenChordix PRIVATE ${ALSA_PC_INCLUDE_DIRS})
     target_link_libraries(OpenChordix PRIVATE ${ALSA_PC_LIBRARIES})
  endif()
else()
   message(FATAL_ERROR "Cannot link rtaudio - library not found.")
endif()

# Link Threads only if found manually (i.e., Unix without vcpkg/CMake config handling it)
if(UNIX AND NOT rtaudio_FOUND AND Threads_FOUND)
   message(STATUS "Linking Threads manually")
   target_link_libraries(OpenChordix PRIVATE Threads::Threads)
endif()


# --- Include Source Directory ---
target_include_directories(OpenChordix PRIVATE src)


# --- Compile Definitions (Platform Specific) ---
# if(UNIX AND CMAKE_SYSTEM_NAME MATCHES "Linux" AND RTAUDIO_PC_FOUND)
    # target_compile_definitions(OpenChordix PRIVATE ${RTAUDIO_PC_CFLAGS_OTHER})
    # target_compile_definitions(OpenChordix PRIVATE __LINUX_ALSA__) 
# endif()


# --- Optional: Show found variables for debugging ---
message(STATUS "Project Source Dir: ${PROJECT_SOURCE_DIR}")
if(rtaudio_FOUND)
  try_get_target_property(RTAUDIO_INCLUDE_DIRS rtaudio::rtaudio INTERFACE_INCLUDE_DIRECTORIES)
  try_get_target_property(RTAUDIO_LINK_LIBRARIES rtaudio::rtaudio INTERFACE_LINK_LIBRARIES)
  message(STATUS "Found RtAudio includes (via target): ${RTAUDIO_INCLUDE_DIRS}")
  message(STATUS "Found RtAudio link libraries (via target): ${RTAUDIO_LINK_LIBRARIES}")
elseif(RTAUDIO_PC_FOUND)
   message(STATUS "Found RtAudio includes (via pkg-config): ${RTAUDIO_PC_INCLUDE_DIRS}")
   message(STATUS "Found RtAudio libraries (via pkg-config): ${RTAUDIO_PC_LIBRARIES}")
endif()
# if(UNIX AND Threads_FOUND)
#   message(STATUS "Found Threads libraries: ${Threads_LIBRARIES}")
# endif()
