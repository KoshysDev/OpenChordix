add_library(openchordix_gui OBJECT_DEPENDS

)

target_include_directories(openchordix_gui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(bgfx CONFIG REQUIRED)
else()
    # Linux: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_PC sdl2)
    pkg_check_modules(BGFX_PC bgfx)
endif()

# --- Link Libraries ---
target_link_libraries(openchordix_gui PUBLIC
    # SDL2
    $<$<TARGET_EXISTS:SDL2::SDL2>:SDL2::SDL2>
    $<$<AND:$<NOT:$<TARGET_EXISTS:SDL2::SDL2>>,$<BOOL:${SDL2_PC_FOUND}>>:${SDL2_PC_LIBRARIES}>

    # BGFX
    $<$<TARGET_EXISTS:bgfx::bgfx>:bgfx::bgfx>
    $<$<AND:$<NOT:$<TARGET_EXISTS:bgfx::bgfx>>,$<BOOL:${BGFX_PC_FOUND}>>:${BGFX_PC_LIBRARIES}>
)

message(STATUS "GUI CMake: Configured openchordix_gui object library")
