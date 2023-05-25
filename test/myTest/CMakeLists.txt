project(lsmt-my-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_for_report test_main.cpp ../../src/kvstore.cc)

add_test(NAME TestReport COMMAND test_for_report)