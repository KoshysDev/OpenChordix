add_executable(OpenChordix
    main.cpp
    AppController.cpp
    DesktopIntegration.cpp
    console/ConsoleFlow.cpp
    console/ConsolePrompter.cpp
    GraphicsFlow.cpp
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
set(OPENCHORDIX_EMBEDDED_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedAssets.cpp)
add_custom_command(
    OUTPUT ${OPENCHORDIX_EMBEDDED_ASSETS_CPP}
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_DIR=${OPENCHORDIX_ASSET_ROOT}
        -DOUTPUT_CPP=${OPENCHORDIX_EMBEDDED_ASSETS_CPP}
        -P ${CMAKE_SOURCE_DIR}/cmake/GenerateEmbeddedAssets.cmake
    DEPENDS ${OPENCHORDIX_ASSET_FILES} ${CMAKE_SOURCE_DIR}/cmake/GenerateEmbeddedAssets.cmake
    VERBATIM
)
add_custom_target(OpenChordixEmbeddedAssets DEPENDS ${OPENCHORDIX_EMBEDDED_ASSETS_CPP})
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
set(APP_ICON_PNG_PATH ${APP_ICON_DIR}/${APP_ICON_NAME}.png)
set(APP_BANNER_PATH ${APP_ICON_DIR}/banner.png)
set(APP_ICON_FILES "")
set(APP_ICON_THEME_NAME openchordix)

if(EXISTS "${APP_ICON_ICO_PATH}")
    message(STATUS "App icon (.ico) found at ${APP_ICON_ICO_PATH}")
    target_compile_definitions(OpenChordix PRIVATE OPENCHORDIX_APP_ICON_ICO_PATH="${APP_ICON_ICO_PATH}")
    list(APPEND APP_ICON_FILES "${APP_ICON_ICO_PATH}")
endif()

if(EXISTS "${APP_ICON_PNG_PATH}")
    message(STATUS "App icon (.png) found at ${APP_ICON_PNG_PATH}")
    target_compile_definitions(OpenChordix PRIVATE OPENCHORDIX_APP_ICON_PNG_PATH="${APP_ICON_PNG_PATH}")
    list(APPEND APP_ICON_FILES "${APP_ICON_PNG_PATH}")
endif()

if(EXISTS "${APP_BANNER_PATH}")
    message(STATUS "App banner found at ${APP_BANNER_PATH}")
    target_compile_definitions(OpenChordix PRIVATE OPENCHORDIX_BANNER_PATH="${APP_BANNER_PATH}")
    list(APPEND APP_ICON_FILES "${APP_BANNER_PATH}")
endif()

set(OPENCHORDIX_DESKTOP_EXEC "OpenChordix")
set(OPENCHORDIX_DESKTOP_ICON "${APP_ICON_THEME_NAME}")

if(APP_ICON_FILES)
    add_custom_command(TARGET OpenChordix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:OpenChordix>/assets/icons"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${APP_ICON_FILES} "$<TARGET_FILE_DIR:OpenChordix>/assets/icons"
        VERBATIM
    )

    if(WIN32)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc.in ${CMAKE_CURRENT_BINARY_DIR}/appicon.rc @ONLY)
        target_sources(OpenChordix PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/appicon.rc)
    endif()
else()
    message(WARNING "App icon not found in ${APP_ICON_DIR}")
endif()

install(TARGETS OpenChordix RUNTIME DESTINATION bin)

target_link_libraries(OpenChordix PRIVATE
    openchordix_core
    openchordix_renderer
    openchordix_devtools
)

message(STATUS "App CMake: Configured OpenChordix executable")
