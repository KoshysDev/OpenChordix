add_library(openchordix_devtools STATIC
    DevConsole.cpp
    CommandRegistry.cpp
    commands/CommandList.cpp
    commands/HelpCommand.cpp
    commands/ClearCommand.cpp
    commands/QuitCommand.cpp
    commands/SceneCommands.cpp
    commands/ModelCommands.cpp
)

set(BGFX_ROOT ${CMAKE_SOURCE_DIR}/external)
set(BGFX_DIR ${BGFX_ROOT}/bgfx)

target_include_directories(openchordix_devtools PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${BGFX_DIR}/include>
    $<BUILD_INTERFACE:${BGFX_ROOT}/bx/include>
    $<BUILD_INTERFACE:${BGFX_ROOT}/bimg/include>
    $<BUILD_INTERFACE:${BGFX_DIR}/3rdparty>
    $<BUILD_INTERFACE:${BGFX_DIR}/examples/common>
)

target_compile_definitions(openchordix_devtools PRIVATE
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:BX_CONFIG_DEBUG=0>
)

message(STATUS "Devtools CMake: Configured openchordix_devtools")
