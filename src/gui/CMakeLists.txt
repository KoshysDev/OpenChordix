add_library(openchordix_gui STATIC
    gui.cpp
    gui.h
)

target_include_directories(openchordix_gui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(Threads REQUIRED)

if (WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(bgfx CONFIG REQUIRED)
    target_compile_definitions(openchordix_gui PRIVATE BX_PLATFORM_WINDOWS=1)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_PC sdl2)
    pkg_check_modules(BGFX_PC bgfx)
endif()

target_link_libraries(openchordix_gui PUBLIC
    Threads::Threads
    $<$<TARGET_EXISTS:SDL2::SDL2>:SDL2::SDL2>
    $<$<TARGET_EXISTS:SDL2::SDL2main>:SDL2::SDL2main>
    $<$<TARGET_EXISTS:bgfx::bgfx>:bgfx::bgfx>
    $<$<PLATFORM_ID:Linux>:"dl;pthread">
)
