cmake_minimum_required(VERSION 3.16)
project(chip8)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  SDL
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.9
)

FetchContent_MakeAvailable(SDL)

add_executable(
  chip8
  src/main.c
  src/cpu.c
  src/config.c
)

target_include_directories(chip8 PRIVATE src)
target_link_libraries(chip8 PRIVATE SDL2::SDL2main SDL2::SDL2)