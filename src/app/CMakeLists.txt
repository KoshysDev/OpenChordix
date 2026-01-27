add_executable(OpenChordix
    main.cpp
    AppController.cpp
    console/ConsoleFlow.cpp
    console/ConsolePrompter.cpp
    platform/ImGuiPlatformBridge.cpp
    scenes/SceneCatalog.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/GraphicsFlow.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/GraphicsContext.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/DisplayManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/audio/AudioSession.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/AnimatedUI.cpp
    scenes/IntroScene.cpp
    scenes/AudioSetupScene.cpp
    scenes/MainMenuScene.cpp
    scenes/TrackSelectScene.cpp
    scenes/SettingsScene.cpp
    scenes/TunerScene.cpp
    scenes/TestScene.cpp
    ${CMAKE_SOURCE_DIR}/src/core/track/TrackCatalogMemory.cpp
    ${CMAKE_SOURCE_DIR}/src/core/score/ScoreServiceMemory.cpp
    settings/GraphicsConfig.cpp
    settings/DisplaySettingsController.cpp
    ui/ModalDialog.cpp
    ui/DeviceSelector.cpp
    ui/UILayout.cpp
)

set(OPENCHORDIX_ASSET_ROOT ${CMAKE_SOURCE_DIR}/assets)
file(GLOB_RECURSE OPENCHORDIX_ASSET_FILES CONFIGURE_DEPENDS "${OPENCHORDIX_ASSET_ROOT}/*")
if(NOT OPENCHORDIX_ASSET_FILES)
    message(FATAL_ERROR "No assets found under ${OPENCHORDIX_ASSET_ROOT}. Embedded assets are required.")
endif()
if(NOT OPENCHORDIX_GENERATED_ASSET_ROOT)
    set(OPENCHORDIX_GENERATED_ASSET_ROOT "${CMAKE_BINARY_DIR}/generated-assets")
endif()
file(MAKE_DIRECTORY "${OPENCHORDIX_GENERATED_ASSET_ROOT}")
set(OPENCHORDIX_EMBEDDED_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedAssets.cpp)
add_custom_command(
    OUTPUT ${OPENCHORDIX_EMBEDDED_ASSETS_CPP}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_DIRS=${OPENCHORDIX_ASSET_ROOT}\;${OPENCHORDIX_GENERATED_ASSET_ROOT}
        -DOUTPUT_CPP=${OPENCHORDIX_EMBEDDED_ASSETS_CPP}
        -P ${CMAKE_SOURCE_DIR}/cmake/GenerateEmbeddedAssets.cmake
    DEPENDS ${OPENCHORDIX_ASSET_FILES} ${OPENCHORDIX_SHADER_OUTPUTS} ${CMAKE_SOURCE_DIR}/cmake/GenerateEmbeddedAssets.cmake
    VERBATIM
)
add_custom_target(OpenChordixEmbeddedAssets DEPENDS ${OPENCHORDIX_EMBEDDED_ASSETS_CPP})
if(TARGET OpenChordixShaders)
    add_dependencies(OpenChordixEmbeddedAssets OpenChordixShaders)
endif()
add_dependencies(OpenChordix OpenChordixEmbeddedAssets)
target_sources(OpenChordix PRIVATE ${OPENCHORDIX_EMBEDDED_ASSETS_CPP})

set(BGFX_ROOT ${CMAKE_SOURCE_DIR}/external)
set(BX_DIR ${BGFX_ROOT}/bx)
set(BIMG_DIR ${BGFX_ROOT}/bimg)
set(BGFX_DIR ${BGFX_ROOT}/bgfx)

target_include_directories(OpenChordix PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/app/scenes
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${CMAKE_SOURCE_DIR}/src/graphics/assets
    ${BGFX_DIR}/include
    ${BGFX_DIR}/examples/common
    ${BGFX_DIR}/3rdparty
    ${BX_DIR}/include
    ${BIMG_DIR}/include
    ${BIMG_DIR}/3rdparty
)

target_compile_definitions(OpenChordix PRIVATE
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:BX_CONFIG_DEBUG=0>
    OPENCHORDIX_VERSION="${PROJECT_VERSION}"
    OPENCHORDIX_OS="${CMAKE_SYSTEM_NAME}"
)

set(APP_ICON_DIR ${CMAKE_SOURCE_DIR}/assets/icons)
set(APP_ICON_NAME AppIcon)
set(APP_ICON_ICO_PATH ${APP_ICON_DIR}/${APP_ICON_NAME}.ico)

if(WIN32)
    if(EXISTS "${APP_ICON_ICO_PATH}")
        message(STATUS "App icon (.ico) found at ${APP_ICON_ICO_PATH}")
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc.in ${CMAKE_CURRENT_BINARY_DIR}/appicon.rc @ONLY)
        target_sources(OpenChordix PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/appicon.rc)
    else()
        message(WARNING "App icon (.ico) not found at ${APP_ICON_ICO_PATH}")
    endif()
endif()

install(TARGETS OpenChordix RUNTIME DESTINATION bin)

target_link_libraries(OpenChordix PRIVATE
    openchordix_core
    openchordix_renderer
    openchordix_devtools
)

message(STATUS "App CMake: Configured OpenChordix executable")
