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
    ${BIMG_DIR}/3rdparty
)

target_compile_definitions(OpenChordix PRIVATE
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:BX_CONFIG_DEBUG=0>
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

# Install targets for desktop integration (icons + desktop entry)
install(TARGETS OpenChordix RUNTIME DESTINATION bin)

if(EXISTS "${APP_ICON_PNG_PATH}")
    set(INSTALLED_DESKTOP_FILE ${CMAKE_CURRENT_BINARY_DIR}/openchordix.desktop)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/openchordix.desktop.in ${INSTALLED_DESKTOP_FILE} @ONLY)
    install(FILES ${INSTALLED_DESKTOP_FILE} DESTINATION share/applications)

    install(FILES ${APP_ICON_PNG_PATH}
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME ${APP_ICON_THEME_NAME}.png)
    install(FILES ${APP_ICON_PNG_PATH}
        DESTINATION share/icons/hicolor/512x512/apps
        RENAME ${APP_ICON_THEME_NAME}.png)
else()
    message(WARNING "PNG icon not found at ${APP_ICON_PNG_PATH}; desktop entry/icon install skipped")
endif()

# Generate a runnable desktop file alongside the built binary for file managers
set(RUNTIME_ICON_FILE "")
if(EXISTS "${APP_ICON_PNG_PATH}")
    set(RUNTIME_ICON_FILE "${APP_ICON_NAME}.png")
elseif(EXISTS "${APP_ICON_ICO_PATH}")
    set(RUNTIME_ICON_FILE "${APP_ICON_NAME}.ico")
endif()

if(RUNTIME_ICON_FILE)
    file(GENERATE
        OUTPUT "$<TARGET_FILE_DIR:OpenChordix>/OpenChordix.desktop"
        CONTENT "[Desktop Entry]\nName=OpenChordix\nComment=OpenChordix Pitch Detection\nExec=$<TARGET_FILE:OpenChordix>\nIcon=$<TARGET_FILE_DIR:OpenChordix>/assets/icons/${RUNTIME_ICON_FILE}\nTerminal=false\nType=Application\nCategories=AudioVideo;Audio;\n")
    add_custom_command(TARGET OpenChordix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env chmod +x "$<TARGET_FILE_DIR:OpenChordix>/OpenChordix.desktop")
endif()

target_link_libraries(OpenChordix PRIVATE
    openchordix_core
    openchordix_renderer
)

message(STATUS "App CMake: Configured OpenChordix executable")
