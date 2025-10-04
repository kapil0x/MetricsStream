# Thread pool library
add_library(thread_pool_lib
    thread_pool.cpp
)

target_include_directories(thread_pool_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# HTTP server library
add_library(http_server_lib
    http_server.cpp
)

target_include_directories(http_server_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(http_server_lib
    thread_pool_lib
)

# Common utilities library (placeholder for future shared code)
add_library(common_lib
    common.cpp
)

target_include_directories(common_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Ingestion service library
add_library(ingestion_lib
    ingestion_service.cpp
)

target_include_directories(ingestion_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ingestion_lib
    http_server_lib
    common_lib
)