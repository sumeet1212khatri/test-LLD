cmake_minimum_required(VERSION 3.16)
project(hft_simulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Aggressive optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Download httplib single header if not present
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/httplib.h")
  message(STATUS "Downloading httplib.h...")
  file(DOWNLOAD
    "https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h"
    "${CMAKE_SOURCE_DIR}/src/httplib.h"
    SHOW_PROGRESS
  )
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)

set(SOURCES
  src/core/order_book.cpp
  src/core/simulator.cpp
  src/risk/risk_manager.cpp
  src/journal/journal.cpp
  src/main.cpp
)

add_executable(hft_server ${SOURCES})

target_compile_options(hft_server PRIVATE
  -Wall -Wextra -Wpedantic
  -pthread
)

target_link_libraries(hft_server PRIVATE pthread)

# Tests
add_executable(hft_test
  src/core/order_book.cpp
  src/core/simulator.cpp
  src/risk/risk_manager.cpp
  src/journal/journal.cpp
  tests/test_main.cpp
)
target_link_libraries(hft_test PRIVATE pthread)

add_executable(hft_engine
  src/core/order_book.cpp
  src/core/simulator.cpp
  src/risk/risk_manager.cpp
  src/journal/journal.cpp
  src/engine_cli.cpp
)
target_link_libraries(hft_engine PRIVATE pthread)

install(TARGETS hft_engine DESTINATION bin)