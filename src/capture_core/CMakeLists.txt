cmake_minimum_required(VERSION 3.10)
project(capture_core VERSION "0.1.4")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER g++)

add_definitions(-std=gnu++11 -g)
add_definitions(-Wall -Wextra -Werror -pedantic -Wsign-conversion)
add_definitions(-Wno-unknown-pragmas -Wno-variadic-macros -Wno-format-security)

# Ignore unused parameter for development purposes
add_definitions(-Wno-unused-parameter)

set(CAPTURE_CORE_INSTALL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CAPTURE_CORE_INSTALL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(CAPTURE_CORE_INSTALL_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)

SET(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/usr/local/lib")

configure_file(config.h.in config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR}) # this allows the 'config.h' file to be used throughout the program
include_directories(${CAPTURE_INSTALL_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src)