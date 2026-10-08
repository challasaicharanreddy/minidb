cmake_minimum_required(VERSION 3.16)
project(minidb CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -g -fsanitize=address,undefined)
add_link_options(-fsanitize=address,undefined)

add_library(minidb_lib
  src/storage/disk_manager.cpp
  src/storage/slotted_page.cpp
  src/storage/row_serializer.cpp
  src/storage/page_access.cpp
  src/storage/table_heap.cpp
  src/catalog/catalog.cpp)
target_include_directories(minidb_lib PUBLIC src)

include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(minidb_tests
  tests/disk_manager_test.cpp
  tests/slotted_page_test.cpp
  tests/row_serializer_test.cpp
  tests/table_heap_test.cpp)
target_link_libraries(minidb_tests minidb_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(minidb_tests)