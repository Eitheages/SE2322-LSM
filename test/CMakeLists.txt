project(lsmt-test LANGUAGES CXX)

add_custom_target(make_data_dir ALL COMMAND ${CMAKE_COMMAND} -E make_directory data)

add_compile_options(-O3)

add_subdirectory(myTest)

add_executable(test_bft bloom_filter.cpp)
add_executable(test_sl skip_list.cpp)
add_executable(test_mtb memory_table.cpp)
add_executable(test_kvstore kvstore.cpp ../src/kvstore.cc)
add_executable(correctness correctness.cc ../src/kvstore.cc)

add_test(NAME TestBft COMMAND test_bft)
add_test(NAME TestSkipList COMMAND test_sl)
add_test(NAME TestMemoryTabel COMMAND test_mtb)
add_test(NAME TestKVStore COMMAND test_kvstore)
add_test(NAME TestAll COMMAND correctness)