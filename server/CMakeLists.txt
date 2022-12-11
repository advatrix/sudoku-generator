cmake_minimum_required(VERSION 3.23)
project(server C)

set(CMAKE_C_STANDARD 99)

add_executable(server main.c grid.c grid.h ../sudoku_field.c random_dev.c random_dev.h ../utils.c ../utils.h)
add_executable(worker worker.c worker.h grid.c grid.h)
