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

# Library for core socket functionality
add_library(socket_lib
    src/InetAddress.cpp
    src/Socket.cpp
    src/Epoll.cpp
    src/Channel.cpp
    src/Eventloop.cpp
)
target_include_directories(socket_lib PUBLIC include)

# Main server executable
add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE socket_lib)

# Testing
find_package(Threads REQUIRED)
enable_testing()

file(GLOB TEST_SOURCES "tests/*.cpp")
message(STATUS "test files: ${TEST_SOURCES}")
# Helper function to add tests
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE socket_lib Threads::Threads)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()