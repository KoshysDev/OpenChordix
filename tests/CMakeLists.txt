find_package(Catch2 CONFIG REQUIRED)

add_executable(openchordix_tests
    test_note_converter.cpp
    test_config_store.cpp
    test_embedded_assets.cpp
    test_imgui_platform_bridge.cpp
    test_pitch_detector.cpp
    test_graphics_config.cpp
    test_track_chart_document.cpp
    test_track_editor_math.cpp
    test_track_import.cpp
    test_leaks.cpp
    ${CMAKE_SOURCE_DIR}/src/app/settings/GraphicsConfig.cpp
)

target_link_libraries(openchordix_tests PRIVATE
    openchordix_core
    Catch2::Catch2WithMain
)

option(OPENCHORDIX_TESTS_SANITIZE "Enable Address/Leak sanitizer for tests" OFF)
if(OPENCHORDIX_TESTS_SANITIZE)
    target_compile_options(openchordix_tests PRIVATE -fsanitize=address)
    target_link_options(openchordix_tests PRIVATE -fsanitize=address)
endif()

target_compile_features(openchordix_tests PRIVATE cxx_std_20)
target_include_directories(openchordix_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${CMAKE_SOURCE_DIR}/external/bgfx/include
    ${CMAKE_SOURCE_DIR}/external/bx/include
    ${CMAKE_SOURCE_DIR}/external/bgfx/3rdparty/dear-imgui
    ${CMAKE_SOURCE_DIR}/external/bgfx/examples/common
)

add_test(NAME openchordix_tests COMMAND openchordix_tests)

add_executable(openchordix_console_tests
    test_console_flow.cpp
)

target_link_libraries(openchordix_console_tests PRIVATE
    Catch2::Catch2WithMain
)

target_compile_features(openchordix_console_tests PRIVATE cxx_std_20)
target_include_directories(openchordix_console_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/core
)

add_test(NAME openchordix_console_tests COMMAND openchordix_console_tests)

add_executable(openchordix_app_entry_tests
    test_app_entry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/NoteConverter.cpp
)

target_link_libraries(openchordix_app_entry_tests PRIVATE
    Catch2::Catch2WithMain
)

target_compile_features(openchordix_app_entry_tests PRIVATE cxx_std_20)
target_include_directories(openchordix_app_entry_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/entry_stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/core
)

add_test(NAME openchordix_app_entry_tests COMMAND openchordix_app_entry_tests)

add_executable(openchordix_ui_header_tests
    test_ui_headers.cpp
)

target_link_libraries(openchordix_ui_header_tests PRIVATE
    Catch2::Catch2WithMain
)

target_compile_features(openchordix_ui_header_tests PRIVATE cxx_std_20)
target_include_directories(openchordix_ui_header_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_header_stubs
    ${CMAKE_SOURCE_DIR}/src/app
)

add_test(NAME openchordix_ui_header_tests COMMAND openchordix_ui_header_tests)
