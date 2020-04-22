find_package(Gurobi REQUIRED)

add_library(sudoku board.c checked_alloc.c parser.c list.c history.c backtrack.c lp.c)
target_link_libraries(sudoku PRIVATE Gurobi::Gurobi)
target_include_directories(sudoku PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(sudoku-console main.c)
target_link_libraries(sudoku-console sudoku)
