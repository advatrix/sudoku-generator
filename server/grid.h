//
// Created by rakhmerov on 06.12.2022.
//

#ifndef SERVER_GRID_H
#define SERVER_GRID_H

#include "../sudoku_field.h"

void base_grid(sudoku_field_t* sudoku_field);
sudoku_field_t transpose(sudoku_field_t* source);
sudoku_field_t swap_rows_small(sudoku_field_t *source);
sudoku_field_t swap_columns_small(sudoku_field_t *source);
void remove_cell(sudoku_field_t* source);

typedef sudoku_field_t (*mix_function)(sudoku_field_t*);

#endif //SERVER_GRID_H
