cmake_minimum_required(VERSION 3.10)
project(hw3-tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -g -fsanitize=address,leak,undefined")

add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(tests ${PROJECT_SOURCE_DIR}/gtest.cc)
target_link_libraries(tests gtest)
