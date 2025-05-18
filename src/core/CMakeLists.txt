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

if(WIN32)
    find_package(Aubio CONFIG REQUIRED)
    find_package(RtAudio CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
else()
    # Linux: Use pkg-config instead
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RTAUDIO_PC rtaudio)
    pkg_check_modules(ALSA_PC alsa)
    pkg_check_modules(AUBIO_PC aubio)
    pkg_check_modules(NLOHMANN_JSON_PC nlohmann-json)
endif()        

# --- Link Libraries ---
target_link_libraries(openchordix_core PUBLIC
    Threads::Threads

    # RtAudio
    $<$<TARGET_EXISTS:RtAudio::rtaudio>:RtAudio::rtaudio>
    $<$<AND:$<NOT:$<TARGET_EXISTS:RtAudio::rtaudio>>,$<BOOL:${RTAUDIO_PC_FOUND}>>:${RTAUDIO_PC_LIBRARIES}>
    $<$<AND:$<NOT:$<TARGET_EXISTS:RtAudio::rtaudio>>,$<BOOL:${ALSA_PC_FOUND}>>:${ALSA_PC_LIBRARIES}>

    # Aubio
    $<$<TARGET_EXISTS:aubio::aubio>:aubio::aubio>
    $<$<AND:$<NOT:$<TARGET_EXISTS:aubio::aubio>>,$<BOOL:${AUBIO_PC_FOUND}>>:${AUBIO_PC_LIBRARIES}>

    # JSON
    $<$<TARGET_EXISTS:nlohmann_json::nlohmann_json>:nlohmann_json::nlohmann_json>

    # SDL2
    $<$<TARGET_EXISTS:SDL2::SDL2>:SDL2::SDL2>
)


message(STATUS "Core CMake: Configured openchordix_core object library")