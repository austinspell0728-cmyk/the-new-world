# CMake build for the C++ / raylib prototype. Works on Windows, macOS and
# Linux; raylib is downloaded and built automatically (FetchContent), so
# nothing has to be installed besides a C++17 compiler and CMake.
#
#     cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build --config Release
#     ctest --test-dir build -C Release --output-on-failure   # rule tests
#     cmake --build build --config Release --target run       # play
#
# Options:
#     -DNEW_WORLD_BUILD_GAME=OFF   only the rule tests: no raylib download,
#                                  no window code (fast, for CI or a quick check)
#     -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/path/to/raylib
#                                  use a raylib source folder you already have
#                                  instead of downloading it

cmake_minimum_required(VERSION 3.25)
project(NewWorld LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(NEW_WORLD_BUILD_GAME "Build the game itself (downloads raylib)" ON)

# Our own warning settings, for our code only (not raylib's).
function(new_world_warnings target)
    if(MSVC)
        # /permissive- : standard C++; _CRT_SECURE_NO_WARNINGS: allow sscanf.
        target_compile_options(${target} PRIVATE /W3 /permissive- /utf-8)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# --- Rules: game/ plus the raylib-free platform helpers ------------------------
#
# This library never sees raylib's headers. If anything in it ever includes
# raylib, it stops building, which keeps the layers apart (the tests link
# only this library).

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS src/game/*.cpp)
set(PURE_PLATFORM_SOURCES
    src/platform/BuildingData.cpp
    src/platform/BuildingLayout.cpp
    src/platform/ControlsData.cpp
    src/platform/InputMath.cpp
    src/platform/TreeData.cpp
    src/platform/TreeLayout.cpp
)
add_library(new_world_rules STATIC ${GAME_SOURCES} ${PURE_PLATFORM_SOURCES})
new_world_warnings(new_world_rules)

# --- Tests ------------------------------------------------------------------------

enable_testing()
add_executable(game-tests tests/GameTests.cpp)
target_link_libraries(game-tests PRIVATE new_world_rules)
new_world_warnings(game-tests)
# The tests read data/*.txt, so they run from cpp/.
add_test(NAME game-tests COMMAND game-tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# --- The game ---------------------------------------------------------------------

if(NEW_WORLD_BUILD_GAME)
    include(FetchContent)
    # raylib 6.0, pinned by checksum so every machine builds the same code.
    FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
        URL_HASH SHA256=2b3ee1e2120c7a0796b33062c7e9a694dd8a8caa56a96319ac8c8ecf54a90d0b
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)

    # Window, input and drawing: everything in platform/ that uses raylib.
    # (The glob gives full paths, so the raylib-free files are removed by
    # their full paths too: they are already in new_world_rules.)
    file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS src/platform/*.cpp)
    list(TRANSFORM PURE_PLATFORM_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/ OUTPUT_VARIABLE PURE_PLATFORM_PATHS)
    list(REMOVE_ITEM PLATFORM_SOURCES ${PURE_PLATFORM_PATHS})

    add_executable(new-world src/main.cpp ${PLATFORM_SOURCES})
    target_include_directories(new-world PRIVATE src)
    # raylib's target brings its headers and the system libraries it needs
    # (OpenGL, Cocoa/IOKit on macOS, gdi32/winmm on Windows).
    target_link_libraries(new-world PRIVATE new_world_rules raylib)
    new_world_warnings(new-world)

    # `cmake --build build --target run`: builds and starts the game from
    # cpp/, where it finds data/ and assets/.
    add_custom_target(run
        COMMAND new-world
        DEPENDS new-world
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        USES_TERMINAL
    )
endif()
