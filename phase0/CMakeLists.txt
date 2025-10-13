cmake_minimum_required(VERSION 3.10)
project(Phase0_PoC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the Phase 0 PoC
add_executable(phase0_poc phase0_poc.cpp)

# Link pthread for threading support
target_link_libraries(phase0_poc pthread)

# Build the simple benchmark tool
add_executable(simple_benchmark simple_benchmark.cpp)
target_link_libraries(simple_benchmark pthread)
