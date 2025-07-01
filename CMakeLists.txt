cmake_minimum_required(VERSION 3.31)
project(td2020)

set(CMAKE_CXX_STANDARD 20)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find Poppler with Cairo backend
pkg_check_modules(POPPLER REQUIRED poppler-cpp)

add_executable(td2020 main.cpp)

target_link_libraries(td2020 ${POPPLER_LIBRARIES})
target_include_directories(td2020 PRIVATE ${POPPLER_INCLUDE_DIRS})
target_compile_options(td2020 PRIVATE ${POPPLER_CFLAGS_OTHER})
