find_package(Gurobi REQUIRED)

add_library(sudoku board.c checked_alloc.c parser.c list.c history.c backtrack.c)
target_include_directories(sudoku PRIVATE ${GUROBI_INCLUDE_DIRS})
target_link_libraries(sudoku PUBLIC ${GUROBI_LIBRARY})
target_include_directories(sudoku PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(sudoku-console main.c)
target_link_libraries(sudoku-console sudoku)
