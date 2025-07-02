set(TD2000_INCLUDE_DIR "${TD2000_SOURCE_DIR}/include")
set(TD2000_HEADERS "${TD2000_INCLUDE_DIR}/td2000/td2000.h")
set(TD2000_SOURCES td2000.cpp)

add_library(td2000 ${TD2000_SOURCES} ${TD2000_HEADERS})
target_include_directories(td2000 PUBLIC ${TD2000_INCLUDE_DIR})
target_compile_features(td2000 PUBLIC cxx_std_23)
target_link_libraries(td2000)

# Group header files under the include directory
source_group(
        TREE "${PROJECT_SOURCE_DIR}/include"
        PREFIX "Header Files"
        FILES ${TD2000_HEADERS})