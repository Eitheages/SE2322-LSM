cmake_minimum_required(VERSION 3.0)
project(SE2322-LSM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clangd
include_directories(include)

# Use c++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -D NDEBUG)

include(CTest)
add_subdirectory(test)
enable_testing()