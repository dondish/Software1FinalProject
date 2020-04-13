cmake_minimum_required(VERSION 3.13)
project(sudoku-final C)

add_compile_options(-ansi -Wall -Wextra -Werror -pedantic-errors)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
