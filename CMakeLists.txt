cmake_minimum_required(VERSION 3.27)
project(steganography)

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY  https://github.com/fmtlib/fmt
        GIT_TAG         10.2.1
)

FetchContent_MakeAvailable(fmt)

add_executable(steganography main.cpp)
target_link_libraries(steganography fmt)