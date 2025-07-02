cmake_minimum_required(VERSION 3.25.1)
project(
        TD2000
        VERSION 0.1
        DESCRIPTION ""
        LANGUAGES CXX
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -flto")
endif ()

set(BUILD_SHARED_LIBS OFF)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)
add_subdirectory(apps)

