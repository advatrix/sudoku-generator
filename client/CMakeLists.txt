cmake_minimum_required(VERSION 3.23)
project(client C)

set(CMAKE_C_STANDARD 99)

add_executable(client main.c ../sudoku_field.c)
