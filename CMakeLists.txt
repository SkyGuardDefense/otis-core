cmake_minimum_required(VERSION 3.10)
project(otis_core VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Core Library
add_library(otis_core 
    src/kinematics_math.cpp 
    src/bypasses/ffmpeg_bypass_layer.cpp
)

target_include_directories(otis_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Example Executable
add_executable(mock_sensor_feed examples/mock_sensor_feed.cpp)
target_link_libraries(mock_sensor_feed PRIVATE otis_core)
