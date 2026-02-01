cmake_minimum_required(VERSION 3.16)

project(libgrovedb
  VERSION 1.0.0
  DESCRIPTION "C++20 library wrapping the grovedb_cxx FFI crate."
  HOMEPAGE_URL "https://github.com/dashpay/dash"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
  cmake/grovedb-config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/config/grovedb-config.h
  @ONLY
)

add_library(grovedb src/grovedb.cpp)

target_include_directories(grovedb
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/src
)

target_compile_definitions(grovedb PRIVATE HAVE_GROVEDB_CONFIG)

# FFI headers from grovedb_cxx crate (passed in by the parent project or user)
if(DEFINED GROVEDB_CXX_INCLUDE_DIR)
  target_include_directories(grovedb PRIVATE ${GROVEDB_CXX_INCLUDE_DIR})
endif()

option(GROVEDB_BUILD_TESTS "Build grovedb tests." ON)

if(GROVEDB_BUILD_TESTS)
  find_package(Boost 1.73.0)
  if(Boost_FOUND)
    enable_testing()
    add_subdirectory(src/test)
  else()
    message(STATUS "Boost not found; grovedb tests will not be built.")
  endif()
endif()
