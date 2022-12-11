cmake_minimum_required(VERSION 3.23)
project(sudoku_generator C)

set(CMAKE_C_STANDARD 99)

add_library(sudoku_generator sudoku_field.c utils.h utils.c)
