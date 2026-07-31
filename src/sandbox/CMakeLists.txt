# CMakeLists.txt : the DeepCore sandbox, built anywhere.
#
# The sandbox links nothing from the game -- no d3drm, no engine, no exe-overlaid globals.
# Its only shared source is the header-only DeepCoreLogic.hpp, which is stdlib-only by
# construction. That is what lets the SHIPPING decision logic compile and run on Linux, in
# a container, and in a browser via Emscripten, on a machine that has never seen the game.
#
#   cmake -B build -S . && cmake --build build
#   ./build/sandbox --selftest
#
# Emscripten:
#   emcmake cmake -B build-web -S . && cmake --build build-web
#   (produces sandbox.html / .js / .wasm)

cmake_minimum_required(VERSION 3.16)
project(deepcore_sandbox CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(sandbox
  main.cpp
  Simulation.cpp
  SyntheticLevel.cpp
)

target_include_directories(sandbox PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../openlrr/game
)

if(MSVC)
  target_compile_options(sandbox PRIVATE /W4)
else()
  target_compile_options(sandbox PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EMSCRIPTEN)
  # A browser build is the friendliest possible way to let someone watch this project work
  # without installing a toolchain -- or owning the game, which they may well not.
  set_target_properties(sandbox PROPERTIES SUFFIX ".html")
  target_link_options(sandbox PRIVATE
    "-sALLOW_MEMORY_GROWTH=1"
    "-sEXIT_RUNTIME=0"
    "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
  )
endif()
