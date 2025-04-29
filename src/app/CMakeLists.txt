add_executable(OpenChordix
    main.cpp
)

target_include_directories(OpenChordix PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/gui
)

target_link_libraries(OpenChordix PRIVATE
    openchordix_core
    openchordix_gui
)

message(STATUS "App CMake: Configured OpenChordix executable")