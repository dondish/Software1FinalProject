cmake_minimum_required(VERSION 3.13)
project(sudoku-final C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_compile_options(-ansi -Wall -Wextra -Werror -pedantic-errors -Wno-overlength-strings)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
