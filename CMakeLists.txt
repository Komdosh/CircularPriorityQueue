cmake_minimum_required(VERSION 3.10)
project(CircularPriorityQueue)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS -pthread)

find_package(Boost 1.58.0 COMPONENTS filesystem REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(Mutiqueues main.cpp)
    target_link_libraries(Mutiqueues ${Boost_LIBRARIES})
endif ()

add_executable(CircularPriorityQueue main.cpp CircularPriorityQueue.h)