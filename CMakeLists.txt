cmake_minimum_required(VERSION 3.15)
project(ChessGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Main Chess Game Executable
# ============================================================================

add_executable(chess
    main.cpp
    chess.cpp
)

target_include_directories(chess PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# Google Test Integration
# ============================================================================

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

# ============================================================================
# Unit Tests Executable
# ============================================================================

add_executable(chess_test
    chess_test.cpp
    chess.cpp
)

target_include_directories(chess_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chess_test PRIVATE gtest gtest_main)

# ============================================================================
# Enable Testing
# ============================================================================

enable_testing()
add_test(NAME ChessTests COMMAND chess_test)

# ============================================================================
# Coverage Configuration (optional)
# ============================================================================

option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
    add_link_options(--coverage)
endif()

# ============================================================================
# Build Instructions
# ============================================================================

message(STATUS "Chess Game Build Configuration")
message(STATUS "  Game executable: cmake --build . --target chess")
message(STATUS "  Run game: ./chess < input.txt")
message(STATUS "  Tests: cmake --build . --target chess_test")
message(STATUS "  Run all tests: ctest --verbose")
message(STATUS "  Coverage: cmake -DENABLE_COVERAGE=ON && cmake --build .")
