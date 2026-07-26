cmake_minimum_required(VERSION 3.10)
project(Chip8Emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#tell the compiler where the header files are
include_directories(include)

#tell CMake which files to compile into the .exe
add_executable(chip8 src/main.cpp src/Chip8.cpp)