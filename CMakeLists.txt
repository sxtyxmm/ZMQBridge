cmake_minimum_required(VERSION 3.12)
project(prj1 VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build test suite" ON)

# Platform detection
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 or later
endif()

# Add ZeroMQ library
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "Build ZeroMQ tests" FORCE)
set(WITH_PERF_TOOL OFF CACHE BOOL "Build with perf-tools" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "Build shared ZMQ library" FORCE)
set(BUILD_STATIC ON CACHE BOOL "Build static ZMQ library" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "Enable CPack" FORCE)

# Check if ZeroMQ source exists
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/zeromq/CMakeLists.txt")
    message(WARNING "ZeroMQ source not found in lib/zeromq. You need to download it.")
    message(WARNING "Download libzmq from: https://github.com/zeromq/libzmq/releases")
    message(WARNING "Extract it to: ${CMAKE_CURRENT_SOURCE_DIR}/lib/zeromq/")
endif()

# Add ZeroMQ subdirectory if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/zeromq/CMakeLists.txt")
    add_subdirectory(lib/zeromq)
    set(ZEROMQ_FOUND TRUE)
else()
    set(ZEROMQ_FOUND FALSE)
endif()

# Create the prj1 shared library
add_library(prj1 SHARED
    src/prj1.cpp
)

# Set library properties
set_target_properties(prj1 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/prj1.h
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Define export macro
target_compile_definitions(prj1 PRIVATE PRJ1_EXPORTS)

# Include directories
target_include_directories(prj1
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link ZeroMQ if found
if(ZEROMQ_FOUND)
    # Link against static ZeroMQ library
    if(TARGET libzmq-static)
        target_link_libraries(prj1 PRIVATE libzmq-static)
    elseif(TARGET libzmq)
        target_link_libraries(prj1 PRIVATE libzmq)
    else()
        message(FATAL_ERROR "ZeroMQ library target not found")
    endif()
    
    # Include ZeroMQ headers
    target_include_directories(prj1 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/zeromq/include
    )
else()
    # Try to find system ZeroMQ as fallback
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(ZMQ libzmq)
        if(ZMQ_FOUND)
            target_include_directories(prj1 PRIVATE ${ZMQ_INCLUDE_DIRS})
            target_link_libraries(prj1 PRIVATE ${ZMQ_LIBRARIES})
            message(STATUS "Using system ZeroMQ library")
        else()
            message(FATAL_ERROR "ZeroMQ not found. Please add it to lib/zeromq or install system package.")
        endif()
    else()
        message(FATAL_ERROR "ZeroMQ not found. Please add it to lib/zeromq or install system package.")
    endif()
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(prj1 PRIVATE ws2_32 iphlpapi)
else()
    target_link_libraries(prj1 PRIVATE pthread)
endif()

# Build examples if requested
if(BUILD_EXAMPLES)
    add_executable(prj1_server src/examples/server.cpp)
    target_link_libraries(prj1_server PRIVATE prj1)
    target_include_directories(prj1_server PRIVATE include)
    
    add_executable(prj1_client src/examples/client.cpp)
    target_link_libraries(prj1_client PRIVATE prj1)
    target_include_directories(prj1_client PRIVATE include)
endif()

# Build tests if requested
if(BUILD_TESTS)
    add_executable(prj1_test src/test/test_suite.cpp)
    target_link_libraries(prj1_test PRIVATE prj1)
    target_include_directories(prj1_test PRIVATE include)
    
    # Enable testing
    enable_testing()
    add_test(NAME prj1_test COMMAND prj1_test)
endif()

# Installation rules
install(TARGETS prj1
    EXPORT prj1Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

# Install export targets
install(EXPORT prj1Targets
    FILE prj1Targets.cmake
    NAMESPACE prj1::
    DESTINATION lib/cmake/prj1
)

# Create and install config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/prj1ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/prj1ConfigVersion.cmake"
    DESTINATION lib/cmake/prj1
)
