#ifndef LOTUS_EXAMPLES_SUDOKU_SUDOKU_H
#define LOTUS_EXAMPLES_SUDOKU_SUDOKU_H

#include <string>

//#include <lotus/base/Types.h>
//namespace lotus{
// FIXME, use (const char*, len) for saving memory copying.
std::string solveSudoku(const std::string& puzzle);
const int kCells = 81;
//}
#endif
