cmake_minimum_required(VERSION 3.15) # Bump version for better find_package/vcpkg handling
project(OpenChordix VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find Dependencies ---
find_package(rtaudio REQUIRED)

if(UNIX AND NOT CMAKE_TOOLCHAIN_FILE)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(RTAUDIO_PC rtaudio)
    pkg_check_modules(ALSA_PC alsa)       
  endif()

  # Find PThreads manually on Unix if needed
  find_package(Threads REQUIRED)

endif()


# --- Executable ---
add_executable(OpenChordix
    src/main.cpp
    src/AudioManager.cpp
)


# --- Link Libraries ---
target_link_libraries(OpenChordix PRIVATE rtaudio::rtaudio)

if(UNIX AND NOT CMAKE_TOOLCHAIN_FILE)
  target_link_libraries(OpenChordix PRIVATE Threads::Threads)
  # if(RTAUDIO_PC_FOUND)
  #   target_include_directories(OpenChordix PRIVATE ${RTAUDIO_PC_INCLUDE_DIRS})
  #   target_link_libraries(OpenChordix PRIVATE ${RTAUDIO_PC_LIBRARIES})
  # endif()
  # if(ALSA_PC_FOUND)
  #   target_include_directories(OpenChordix PRIVATE ${ALSA_PC_INCLUDE_DIRS})
  #   target_link_libraries(OpenChordix PRIVATE ${ALSA_PC_LIBRARIES})
  # endif()
endif()


# --- Include Source Directory ---
target_include_directories(OpenChordix PRIVATE src)


# --- Compile Definitions (Platform Specific) ---
if(UNIX AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    # If RtAudio target doesn't define these, add them
    # target_compile_definitions(OpenChordix PRIVATE __LINUX_ALSA__)
    # target_compile_definitions(OpenChordix PRIVATE __UNIX_JACK__)
endif()


# --- Optional: Show found variables for debugging ---
message(STATUS "Project Source Dir: ${PROJECT_SOURCE_DIR}")
get_target_property(RTAUDIO_INCLUDE_DIRS rtaudio::rtaudio INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(RTAUDIO_LINK_LIBRARIES rtaudio::rtaudio INTERFACE_LINK_LIBRARIES)
message(STATUS "Found RtAudio includes (via target): ${RTAUDIO_INCLUDE_DIRS}")
message(STATUS "Found RtAudio link libraries (via target): ${RTAUDIO_LINK_LIBRARIES}")
if(UNIX)
  message(STATUS "Found Threads libraries: ${Threads_LIBRARIES}")
endif()
