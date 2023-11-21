cmake_minimum_required(VERSION 3.16)

project(jpeg_codec CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(main src/main.cpp)
target_include_directories(main PRIVATE include)

# add_library(neon OBJECT neon/neon.cpp neon/ntt_10.cpp neon/intt_10_x10.cpp neon/ntt_9.cpp neon/intt_9_x9.cpp neon/base_mul.cpp neon/mult_low.cpp)
# target_include_directories(neon PUBLIC ../include)
# add_executable("${X}" "${X}.cpp")
# target_link_libraries("${X}" neon)
# target_include_directories("${X}" PUBLIC ../include)
