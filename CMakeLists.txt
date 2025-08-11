cmake_minimum_required(VERSION 3.19.8)
project(OpenChordix VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Top-Level: Adding subdirectories...")

add_subdirectory(src/core)
add_subdirectory(src/gui)
add_subdirectory(src/app)

message(STATUS "Top-Level: Finished processing.")