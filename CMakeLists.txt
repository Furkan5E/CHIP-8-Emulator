cmake_minimum_required(VERSION 3.14)
project(Chip8Emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.5
)
set(SDL_TEST OFF CACHE BOOL "Disable SDL tests" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Disable SDL tests" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Build static SDL2" FORCE)
FetchContent_MakeAvailable(SDL2)

#tell the compiler where the header files are
include_directories(include)

#tell CMake which files to compile into the .exe
add_executable(chip8 src/main.cpp src/Chip8.cpp)
#link the SDL2 library to  executable
target_link_libraries(chip8 PRIVATE SDL2::SDL2-static)