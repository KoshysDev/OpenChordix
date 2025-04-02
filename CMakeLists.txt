cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(OpenChordix VERSION 0.1)

# Add an executable
add_executable(OpenChordix src/main.cpp)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
