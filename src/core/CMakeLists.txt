add_library(openchordix_core STATIC
    AudioManager.cpp
    AudioManager.h
    NoteConverter.cpp
    NoteConverter.h
    PitchDetector.cpp
    PitchDetector.h
    ConfigManager.cpp
    ConfigManager.h
)

target_include_directories(openchordix_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(Threads REQUIRED)

if (WIN32)
    find_package(aubio CONFIG REQUIRED)
    find_package(RtAudio CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RTAUDIO_PC rtaudio)
    pkg_check_modules(ALSA_PC alsa)
    pkg_check_modules(AUBIO_PC aubio)
    pkg_check_modules(NLOHMANN_JSON_PC nlohmann-json)
endif()

target_link_libraries(openchordix_core PUBLIC
    Threads::Threads
    $<$<TARGET_EXISTS:RtAudio::rtaudio>:RtAudio::rtaudio>
    $<$<TARGET_EXISTS:aubio::aubio>:aubio::aubio>
    $<$<TARGET_EXISTS:nlohmann_json::nlohmann_json>:nlohmann_json::nlohmann_json>
    $<$<TARGET_EXISTS:SDL2::SDL2>:SDL2::SDL2>
)