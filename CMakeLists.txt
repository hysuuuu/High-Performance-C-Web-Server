cmake_minimum_required(VERSION 3.10)

project(HighPerformanceWebServer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(server ${SOURCES})