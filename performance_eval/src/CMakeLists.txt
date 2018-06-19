cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_C_COMPILER   icc)
set(CMAKE_CXX_COMPILER icpc)
add_compile_options(-std=c++11)
add_definitions(-DMIC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} xCORE-AVX2 -axMIC-AVX512")

project (artifact)
include_directories("../SSE_API_Package/SSE_Template/")
file(GLOB SOURCES "*.cpp")

foreach(src_full ${SOURCES})
    get_filename_component(src "${src_full}" NAME)
    string(REPLACE ".cpp" "" obj ${src})
    add_executable("${obj}.out" ${src})
    target_link_libraries("${obj}.out" pthread)
endforeach(src_full ${SOURCES})
