find_package(Catch2 CONFIG REQUIRED)

add_executable(openchordix_tests
    test_note_converter.cpp
    test_config_store.cpp
    test_pitch_detector.cpp
    test_graphics_config.cpp
    ${CMAKE_SOURCE_DIR}/src/app/settings/GraphicsConfig.cpp
)

target_link_libraries(openchordix_tests PRIVATE
    openchordix_core
    Catch2::Catch2WithMain
)

target_compile_features(openchordix_tests PRIVATE cxx_std_20)
target_include_directories(openchordix_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/app
)

add_test(NAME openchordix_tests COMMAND openchordix_tests)
