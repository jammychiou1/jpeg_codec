cmake_minimum_required(VERSION 3.16)

project(jpeg_codec CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(lib OBJECT src/mmap.cpp)
target_include_directories(lib PRIVATE include)

add_executable(main src/main.cpp)
target_link_libraries(main lib)
target_include_directories(main PRIVATE include)

# target_include_directories(neon PUBLIC ../include)
# add_executable("${X}" "${X}.cpp")
# target_link_libraries("${X}" neon)
# target_include_directories("${X}" PUBLIC ../include)
