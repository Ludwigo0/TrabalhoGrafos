cmake_minimum_required(VERSION 3.16)
project(TrabalhoGrafos CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
# Otimizacao padrao do CMake Release (-O3). Nao forcar -march=native para o
# binario continuar portavel entre sua maquina e a do professor.

add_library(graflib STATIC
  src/CSRGraph.cpp
  src/BitMatrixGraph.cpp
  src/algorithms.cpp
  src/io.cpp
)
target_include_directories(graflib PUBLIC include)
target_compile_options(graflib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(grafo src/main.cpp)
target_link_libraries(grafo PRIVATE graflib)

add_executable(test_grafo tests/test_grafo.cpp)
target_link_libraries(test_grafo PRIVATE graflib)

enable_testing()
add_test(NAME test_grafo COMMAND test_grafo)
