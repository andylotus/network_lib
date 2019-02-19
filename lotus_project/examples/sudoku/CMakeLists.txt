add_executable(sudoku_solver_basic server_basic.cc sudoku.cc)
target_link_libraries(sudoku_solver_basic lotus_net)

add_executable(sudoku_solver_multiloop server_multiloop.cc sudoku.cc)
target_link_libraries(sudoku_solver_multiloop lotus_net)

add_executable(sudoku_solver_threadpool server_threadpool.cc sudoku.cc)
target_link_libraries(sudoku_solver_threadpool lotus_net)

add_executable(sudoku_solver_hybrid server_hybrid.cc sudoku.cc)
target_link_libraries(sudoku_solver_hybrid lotus_net)