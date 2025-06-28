cmake_minimum_required(VERSION 3.14)
project(sdr_network VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

# Find dependencies
find_package(Boost REQUIRED COMPONENTS
    system
    program_options
    thread
)
find_package(Threads REQUIRED)

# Try to find UHD, but make it optional for initial development
find_package(UHD QUIET)
if(UHD_FOUND)
    message(STATUS "Found UHD: ${UHD_INCLUDE_DIRS}")
    add_definitions(-DHAS_UHD)
else()
    message(STATUS "UHD not found, SDR functionality will be stubbed")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
