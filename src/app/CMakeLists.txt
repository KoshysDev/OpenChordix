add_executable(OpenChordix
    main.cpp
)

target_include_directories(OpenChordix PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(OpenChordix PRIVATE
    openchordix_core
    openchordix_renderer
)

message(STATUS "App CMake: Configured OpenChordix executable")
