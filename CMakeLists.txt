cmake_minimum_required(VERSION 3.26)
project(SE2322-LSM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clangd

# Use c++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall)

include(CTest)
add_subdirectory(test)
enable_testing()

add_executable(lsm test/correctness.cc src/kvstore.cc)