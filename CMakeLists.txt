cmake_minimum_required(VERSION 3.20...3.31)
project(latency_rx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Measurements must run optimized: default to Release when no build type is given
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# compile_commands.json for clangd / clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# ---------------------------------------------------------------------------
# rxcore: platform independent modules (no libgpiod), builds on macOS too
# ---------------------------------------------------------------------------
add_library(rxcore STATIC
    src/config.cpp
)
target_include_directories(rxcore PUBLIC include)

# ---------------------------------------------------------------------------
# tests: run with `ctest --test-dir build`
# ---------------------------------------------------------------------------
enable_testing()

add_executable(config_test tests/config_test.cpp)
target_link_libraries(config_test PRIVATE rxcore)
add_test(NAME config COMMAND config_test)

# ---------------------------------------------------------------------------
# latency_rx: the receiver itself, Linux only (libgpiod, sched_*, AF_PACKET)
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    # GpioToggle uses the libgpiod v2 C++ API (gpiod::line_request, ...)
    pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET "libgpiodcxx>=2.0")

    add_executable(latency_rx
        src/main.cpp
    )
    target_link_libraries(latency_rx PRIVATE rxcore PkgConfig::GPIOD)
else()
    message(STATUS "${CMAKE_SYSTEM_NAME}: building rxcore only, latency_rx needs Linux + libgpiod")
endif()
