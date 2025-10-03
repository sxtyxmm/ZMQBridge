# CMakeLists.txt for prj1
cmake_minimum_required(VERSION 3.15)
project(prj1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output DLL/dylib to build/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Include public headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Vendored ZeroMQ
add_subdirectory(lib/zeromq)
include_directories(lib/zeromq/include)

# prj1 sources
file(GLOB PRJ1_SRC src/*.cpp)

add_library(prj1 SHARED ${PRJ1_SRC})

# Link statically to ZeroMQ
target_link_libraries(prj1 PRIVATE libzmq-static)

# Export symbols for Windows DLL
if(WIN32)
    target_compile_definitions(prj1 PRIVATE PRJ1_EXPORTS)
endif()

# Install public header and library
install(TARGETS prj1 DESTINATION build)
install(FILES include/prj1.h DESTINATION build)

# Examples (optional)
add_executable(prj1_server src/examples/server.cpp)
add_executable(prj1_client src/examples/client.cpp)
target_link_libraries(prj1_server PRIVATE prj1)
target_link_libraries(prj1_client PRIVATE prj1)

# Tests
add_executable(test_req_rep tests/test_req_rep.cpp)
add_executable(test_pub_sub tests/test_pub_sub.cpp)
add_executable(test_push_pull tests/test_push_pull.cpp)
add_executable(test_edge_cases tests/test_edge_cases.cpp)
add_executable(test_multithreading tests/test_multithreading.cpp)

target_link_libraries(test_req_rep PRIVATE prj1)
target_link_libraries(test_pub_sub PRIVATE prj1)
target_link_libraries(test_push_pull PRIVATE prj1)
target_link_libraries(test_edge_cases PRIVATE prj1)
target_link_libraries(test_multithreading PRIVATE prj1)
