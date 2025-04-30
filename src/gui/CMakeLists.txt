add_library(openchordix_gui STATIC
    gui.cpp
    gui.h
)

target_include_directories(openchordix_gui PUBLIC
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include"
)

target_include_directories(openchordix_gui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(bgfx CONFIG REQUIRED)
    find_package(Threads REQUIRED)
    target_compile_definitions(openchordix_gui PRIVATE BX_PLATFORM_WINDOWS=1)
    message(STATUS "GUI CMake: Forcing BX_PLATFORM_WINDOWS=1 for MinGW build.")
else()
    # Linux: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_PC sdl2)
    pkg_check_modules(BGFX_PC bgfx)
endif()

# --- Link Libraries ---
target_link_libraries(openchordix_gui PUBLIC
    Threads::Threads

    # SDL2
    $<$<TARGET_EXISTS:SDL2::SDL2>:SDL2::SDL2>
    $<$<TARGET_EXISTS:SDL2::SDL2main>:SDL2::SDL2main> # Often needed, esp. on Windows
    $<$<AND:$<NOT:$<TARGET_EXISTS:SDL2::SDL2>>,$<BOOL:${SDL2_PC_FOUND}>>:${SDL2_PC_LIBRARIES}>

    # BGFX
    $<$<TARGET_EXISTS:bgfx::bgfx>:bgfx::bgfx>
    $<$<AND:$<NOT:$<TARGET_EXISTS:bgfx::bgfx>>,$<BOOL:${BGFX_PC_FOUND}>>:${BGFX_PC_LIBRARIES}>

    # Platform specific libraries needed by bgfx/SDL
    $<$<PLATFORM_ID:Linux>:"dl;pthread"> # Linux specific
    $<$<AND:$<PLATFORM_ID:Linux>,$<BOOL:${SDL2_PC_FOUND}>>:${SDL2_PC_STATIC_LIBRARIES}>
)

if(SDL2_PC_FOUND)
    target_include_directories(openchordix_gui PUBLIC ${SDL2_PC_INCLUDE_DIRS})
endif()
if(BGFX_PC_FOUND)
    target_include_directories(openchordix_gui PUBLIC ${BGFX_PC_INCLUDE_DIRS})
endif()

get_target_property(GUI_INCLUDE_DIRS openchordix_gui INCLUDE_DIRECTORIES)
message(STATUS "GUI Include Dirs (Final): ${GUI_INCLUDE_DIRS}")

message(STATUS "GUI CMake: Configured openchordix_gui library")
