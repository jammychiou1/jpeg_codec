cmake_minimum_required(VERSION 3.16)

project(jpeg_codec CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-O3)

add_library(lib OBJECT src/mmap.cpp src/dct.cpp)
target_include_directories(lib PRIVATE include)

add_executable(main src/main.cpp)
target_link_libraries(main lib)
target_include_directories(main PRIVATE include)

add_executable(test_mmap test/mmap.cpp)
target_link_libraries(test_mmap lib)
target_include_directories(test_mmap PRIVATE include)

add_executable(test_sse test/sse.cpp)
target_link_libraries(test_sse lib)
target_include_directories(test_sse PRIVATE include)

add_executable(test_dct test/dct.cpp)
target_link_libraries(test_dct lib)
target_include_directories(test_dct PRIVATE include)

add_executable(test_dct_bench test/dct_bench.cpp)
target_link_libraries(test_dct_bench lib)
target_include_directories(test_dct_bench PRIVATE include)
