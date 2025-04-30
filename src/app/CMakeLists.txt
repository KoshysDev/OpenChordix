add_executable(OpenChordix WIN32
    main.cpp
)

target_include_directories(OpenChordix PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/gui
)

if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(Aubio CONFIG REQUIRED)
else()
    if(NOT TARGET SDL2::SDL2)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2_PC REQUIRED sdl2)
    endif()

    if(NOT TARGET Aubio::aubio)
         find_package(PkgConfig REQUIRED)
         pkg_check_modules(AUBIO_PC REQUIRED aubio)
    endif()
endif()

target_link_libraries(OpenChordix PRIVATE
    openchordix_core
    openchordix_gui
    $<$<TARGET_EXISTS:SDL2::SDL2main>:SDL2::SDL2main>
    $<$<TARGET_EXISTS:SDL2::SDL2>:SDL2::SDL2>

    $<$<TARGET_EXISTS:Aubio::aubio>:Aubio::aubio>
    $<$<AND:$<NOT:$<TARGET_EXISTS:Aubio::aubio>>,$<BOOL:${AUBIO_PC_FOUND}>>:${AUBIO_PC_LIBRARIES}>
)

message(STATUS "App CMake: Configured OpenChordix executable")