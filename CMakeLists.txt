cmake_minimum_required(VERSION 3.8)
project(bimap)

set(CMAKE_CXX_STANDARD 17)

#GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
#Threads
find_package(Threads REQUIRED)

set(SOURCE_FILES main.cpp generic.cpp)
add_executable(bimap ${SOURCE_FILES})

target_link_libraries(bimap ${GTEST_BOTH_LIBRARIES})

add_test(test-bimap bimap)