cmake_minimum_required(VERSION 3.16)
project(otis_core VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Include directories ──────────────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/include)

# ── Core library ─────────────────────────────────────────────────────────────
add_library(otis_core
    src/kinematics_math.cpp
    src/bypasses/ffmpeg_bypass_layer.cpp
)
target_include_directories(otis_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ── Examples ─────────────────────────────────────────────────────────────────
add_executable(mock_sensor_feed
    examples/mock_sensor_feed.cpp
)
target_link_libraries(mock_sensor_feed PRIVATE otis_core)

add_executable(mock_telemetry_feed
    examples/mock_telemetry_feed.cpp
)
target_link_libraries(mock_telemetry_feed PRIVATE otis_core)

# ── Compiler warnings ────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(otis_core PRIVATE /W4)
else()
    target_compile_options(otis_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
