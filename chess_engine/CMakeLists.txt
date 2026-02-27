cmake_minimum_required(VERSION 3.14)
project(ChessEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
endif()

# Sources
set(SOURCES
    engine/main.cpp
    engine/board/Board.cpp
    engine/movegen/MoveGen.cpp
    engine/eval/Eval.cpp
    engine/search/Search.cpp
    engine/cli/CLI.cpp
    engine/util/PGN.cpp
)

add_executable(chess_engine ${SOURCES})
target_include_directories(chess_engine PRIVATE engine)

# Tests
enable_testing()
add_executable(perft_test tests/perft_test.cpp engine/board/Board.cpp engine/movegen/MoveGen.cpp)
target_include_directories(perft_test PRIVATE engine)
add_test(NAME PerftTest COMMAND perft_test)
