cmake_minimum_required(VERSION 3.14)
project(hello_pybind)
add_subdirectory(pybind11)
add_subdirectory(matplotplusplus)
set(CMAKE_CXX_STANDARD 17)
include_directories("C:/Users/kutom/Desktop/prjk3/pybind11/include")
include_directories("C:/Users/kutom/AppData/Local/Programs/Python/Python311/include")
include_directories("C:/Users/kutom/Desktop/prjk3/matplotplusplus/source/matplot")
include_directories("C:/Users/kutom/Desktop/prjk3/pybind11/include")
pybind11_add_module(example example.cpp)

target_link_libraries(example PUBLIC matplot)