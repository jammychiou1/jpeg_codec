cmake_minimum_required(VERSION 3.16)

project(jpeg_codec CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-O3)
add_compile_options(-mssse3)

add_library(lib OBJECT src/mmap.cpp)
target_include_directories(lib PRIVATE include)

add_executable(main src/main.cpp)
target_link_libraries(main lib)
target_include_directories(main PRIVATE include)

add_executable(test_mmap test/mmap.cpp)
target_link_libraries(test_mmap lib)
target_include_directories(test_mmap PRIVATE include)

add_executable(test_sse2 test/sse2.cpp)
target_link_libraries(test_sse2 lib)
target_include_directories(test_sse2 PRIVATE include)

add_executable(test_idct_1d_schoolbook test/idct_1d_schoolbook.cpp)
target_link_libraries(test_idct_1d_schoolbook lib)
target_include_directories(test_idct_1d_schoolbook PRIVATE include)

add_executable(test_idct_1d_fft test/idct_1d_fft.cpp)
target_link_libraries(test_idct_1d_fft lib)
target_include_directories(test_idct_1d_fft PRIVATE include)

add_executable(test_idct_schoolbook test/idct_schoolbook.cpp)
target_link_libraries(test_idct_schoolbook lib)
target_include_directories(test_idct_schoolbook PRIVATE include)

add_executable(test_idct_schoolbook_bench test/idct_schoolbook_bench.cpp)
target_link_libraries(test_idct_schoolbook_bench lib)
target_include_directories(test_idct_schoolbook_bench PRIVATE include)

add_executable(test_idct_fft test/idct_fft.cpp)
target_link_libraries(test_idct_fft lib)
target_include_directories(test_idct_fft PRIVATE include)

add_executable(test_idct_fft_bench test/idct_fft_bench.cpp)
target_link_libraries(test_idct_fft_bench lib)
target_include_directories(test_idct_fft_bench PRIVATE include)

add_executable(test_idct_ssse3 test/idct_ssse3.cpp)
target_link_libraries(test_idct_ssse3 lib)
target_include_directories(test_idct_ssse3 PRIVATE include)

add_executable(test_idct_ssse3_bench test/idct_ssse3_bench.cpp)
target_link_libraries(test_idct_ssse3_bench lib)
target_include_directories(test_idct_ssse3_bench PRIVATE include)
