cmake_minimum_required(VERSION 3.18)
project(HFT_OrderBook VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Compiler Flags ───────────────────────────────────────────────────────────
if(MSVC)
    set(HFT_FLAGS /O2 /GL /Gy /GS- /arch:AVX2 /fp:fast /W4)
    set(HFT_LINK_FLAGS /LTCG /OPT:REF /OPT:ICF)
else()
    # GCC / Clang
    set(HFT_FLAGS
        -O3
        -march=native
        -mtune=native
        -funroll-loops
        -ffast-math
        -finline-functions
        -fomit-frame-pointer
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
    set(HFT_LINK_FLAGS "")
endif()

# ─── Threading ────────────────────────────────────────────────────────────────
find_package(Threads REQUIRED)

# ─── Include paths ────────────────────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/include)

# ─── Library: core orderbook (shared) ────────────────────────────────────────
add_library(orderbook_lib STATIC
    src/Orderbook.cpp
)
target_compile_options(orderbook_lib PRIVATE ${HFT_FLAGS})
target_link_libraries(orderbook_lib PUBLIC Threads::Threads)
target_include_directories(orderbook_lib PUBLIC include)

# ─── Executable: benchmark ────────────────────────────────────────────────────
add_executable(benchmark src/main.cpp)
target_compile_options(benchmark PRIVATE ${HFT_FLAGS})
target_link_libraries(benchmark PRIVATE orderbook_lib)

# ─── Executable: tests ────────────────────────────────────────────────────────
add_executable(run_tests tests/test_orderbook.cpp)
target_compile_options(run_tests PRIVATE ${HFT_FLAGS})
target_link_libraries(run_tests PRIVATE orderbook_lib)

# ─── Install ─────────────────────────────────────────────────────────────────
install(TARGETS benchmark run_tests
    RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/hft_orderbook)

# ─── Print config summary ─────────────────────────────────────────────────────
message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
