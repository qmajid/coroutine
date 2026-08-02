cmake_minimum_required(VERSION 3.20)
project(coroutin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if (CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY
        "${CLANG_TIDY_EXE};-warnings-as-errors=*")
else()
    message(WARNING "clang-tidy not found — skipping clang-tidy checks")
endif()


add_executable(coroutin
    main.cpp
    src/coroutine_task_queue.cpp
    src/await.cpp
)

target_include_directories(coroutin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# pthread is needed for std::thread/std::jthread on Linux
find_package(Threads REQUIRED)
target_link_libraries(coroutin PRIVATE Threads::Threads)