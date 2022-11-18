cmake_minimum_required(VERSION 3.12)

project(cache)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# cmake3 .. -GNinja -DTBB_TEST=OFF TBB_COMMON_COMPILE_FLAGS=-std=c++17 -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
link_directories(oneTBB/build/gnu_10.2_cxx11_64_relwithdebinfo)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -Wextra")
add_executable(${PROJECT_NAME} main.cpp lru.hpp)
target_include_directories(${PROJECT_NAME} PRIVATE oneTBB/include)
target_link_libraries(${PROJECT_NAME} PRIVATE tbb)


