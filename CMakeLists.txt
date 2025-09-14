cmake_minimum_required(VERSION 3.16)
project(MetricStream VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Create main executable
add_executable(metricstream_server
    src/main.cpp
)

target_link_libraries(metricstream_server
    ingestion_lib
    common_lib
    Threads::Threads
)

# Create test client
add_executable(test_client
    test_client.cpp
)

target_link_libraries(test_client
    Threads::Threads
)