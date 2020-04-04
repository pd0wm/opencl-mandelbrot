cmake_minimum_required(VERSION 3.10)

project(EPSN)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCL REQUIRED)

add_compile_options(-Wall -Wextra -pedantic)
add_compile_options(-O2)

add_executable(main main.cc assert_cl.cc)
target_link_libraries(main OpenCL::OpenCL)
