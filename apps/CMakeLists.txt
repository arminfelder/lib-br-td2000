cmake_minimum_required(VERSION 3.25.1)

add_executable(app app.cpp)
target_compile_features(app PRIVATE cxx_std_23)

target_link_libraries(app PRIVATE td2000)
