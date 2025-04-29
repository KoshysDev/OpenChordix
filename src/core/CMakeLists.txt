add_library(openchordix_core STATIC
    AudioManager.cpp
    AudioManager.h
    NoteConverter.cpp
    NoteConverter.h
    PitchDetector.cpp
    PitchDetector.h
)

target_include_directories(openchordix_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(Aubio CONFIG REQUIRED)
find_package(RtAudio CONFIG REQUIRED)

# --- Link Libraries ---
target_link_libraries(openchordix_core PUBLIC
    Threads::Threads
    Aubio::aubio
    RtAudio::rtaudio

    # RtAudio Linking
    $<$<TARGET_EXISTS:RtAudio::rtaudio>:RtAudio::rtaudio> # Use target if available (Windows/vcpkg)
    $<$<AND:$<NOT:$<TARGET_EXISTS:RtAudio::rtaudio>>,$<BOOL:${RTAUDIO_PC_FOUND}>>:${RTAUDIO_PC_LIBRARIES}> # Use pkg-config libs if target missing (Linux)
    $<$<AND:$<NOT:$<TARGET_EXISTS:RtAudio::rtaudio>>,$<BOOL:${ALSA_PC_FOUND}>>:${ALSA_PC_LIBRARIES}> # Link ALSA if using pkg-config rtaudio

    # Aubio Linking
    $<$<TARGET_EXISTS:aubio::aubio>:aubio::aubio> # Use target if available (Windows/vcpkg)
    $<$<AND:$<NOT:$<TARGET_EXISTS:aubio::aubio>>,$<BOOL:${AUBIO_PC_FOUND}>>:${AUBIO_PC_LIBRARIES}> # Use pkg-config libs if target missing (Linux)
)

message(STATUS "Core CMake: Configured openchordix_core object library")