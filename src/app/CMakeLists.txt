add_executable(OpenChordix
    main.cpp
    AppController.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/GraphicsContext.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AudioSession.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/AnimatedUI.cpp
    scenes/IntroScene.cpp
    scenes/AudioSetupScene.cpp
    scenes/MainMenuScene.cpp
)

set(BGFX_ROOT ${CMAKE_SOURCE_DIR}/external)
set(BX_DIR ${BGFX_ROOT}/bx)
set(BIMG_DIR ${BGFX_ROOT}/bimg)
set(BGFX_DIR ${BGFX_ROOT}/bgfx)

target_include_directories(OpenChordix PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/app/scenes
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${BGFX_DIR}/include
    ${BGFX_DIR}/examples/common
    ${BGFX_DIR}/3rdparty
    ${BX_DIR}/include
    ${BIMG_DIR}/include
)

target_compile_definitions(OpenChordix PRIVATE
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:BX_CONFIG_DEBUG=0>
)

target_link_libraries(OpenChordix PRIVATE
    openchordix_core
    openchordix_renderer
)

message(STATUS "App CMake: Configured OpenChordix executable")
