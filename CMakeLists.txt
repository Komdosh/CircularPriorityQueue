cmake_minimum_required(VERSION 3.10)
project(CircularPriorityQueue)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS -pthread)
set(CMAKE_CXX_FLAGS_RELEASE -O3)

find_package(Boost 1.58.0 COMPONENTS filesystem REQUIRED)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(CircularPriorityQueue main.cpp)
    target_link_libraries(CircularPriorityQueue ${Boost_LIBRARIES})
endif ()

#add_executable(CircularPriorityQueue main.cpp CircularPriorityQueue.h)