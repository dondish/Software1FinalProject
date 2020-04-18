function(test_module NAME)
    set(TEST_NAME sudoku-test-${NAME})
    add_executable(${TEST_NAME} test_${NAME}.c)
    target_link_libraries(${TEST_NAME} sudoku)
    add_test(NAME ${NAME} COMMAND ${TEST_NAME})
endfunction()

test_module(board)
test_module(parser)
