# Tests will be added here later
# For now, just create an empty test file to satisfy CMake

add_executable(placeholder_test
    placeholder_test.cpp
)

target_link_libraries(placeholder_test
    http_server_lib
    common_lib
)

add_test(NAME placeholder COMMAND placeholder_test)