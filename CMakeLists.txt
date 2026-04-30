cmake_minimum_required(VERSION 3.20)
project(coroutin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(coroutin
    main.cpp
    coroutine_task_queue.cpp
)

target_include_directories(coroutin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# pthread is needed for std::thread/std::jthread on Linux
find_package(Threads REQUIRED)
target_link_libraries(coroutin PRIVATE Threads::Threads)