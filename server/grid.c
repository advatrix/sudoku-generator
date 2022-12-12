//
// Created by rakhmerov on 06.12.2022.
//

#include "grid.h"
#include "random_dev.h"

void base_grid(sudoku_field_t *sudoku_field) {
    for (short i = 0; i < FIELD_SIDE_LENGTH; ++i) {
        for (short j = 0; j < FIELD_SIDE_LENGTH; ++j) {
            sudoku_field->cells[i][j].value =
                    ((i * REGION_SIDE_LENGTH) + (i / REGION_SIDE_LENGTH + j)) % FIELD_SIDE_LENGTH + 1;
        }
    }
}

sudoku_field_t transpose(sudoku_field_t *source) {
    sudoku_field_t result = *source;
    for (int i = 0; i < FIELD_SIDE_LENGTH; ++i) {
        for (int j = 0; j < FIELD_SIDE_LENGTH; ++j) {
            result.cells[j][i] = source->cells[i][j];
        }
    }
    return result;
}


void copy_row(sudoku_cell_t* to, sudoku_cell_t* from) {
    for (int i = 0; i < FIELD_SIDE_LENGTH; ++i) {
        to[i].value = from[i].value;
    }
}

sudoku_field_t swap_rows_small(sudoku_field_t *source) {
    sudoku_field_t result = *source;
    int region_num = random_number(REGION_SIDE_LENGTH);
    int first_region_row_num = random_number(REGION_SIDE_LENGTH);
    int first_row_num = region_num * REGION_SIDE_LENGTH + first_region_row_num;

    int second_region_row_num = random_number(REGION_SIDE_LENGTH);
    while (first_region_row_num == second_region_row_num) {
        second_region_row_num = random_number(REGION_SIDE_LENGTH);
    }
    int second_row_num = region_num * REGION_SIDE_LENGTH + second_region_row_num;

    copy_row(result.cells[first_row_num], source->cells[second_row_num]);
    copy_row(result.cells[second_row_num], source->cells[first_row_num]);

    return result;
}

sudoku_field_t swap_columns_small(sudoku_field_t *source) {
    sudoku_field_t result = transpose(source);
    sudoku_field_t swapped = swap_rows_small(&result);
    return transpose(&swapped);
}

void remove_cell(sudoku_field_t* source) {
    // generate number of cell to remove
    int row_num = random_number(FIELD_SIDE_LENGTH);
    int col_num = random_number(FIELD_SIDE_LENGTH);

    while (source->cells[row_num][col_num].value == SUDOKU_EMPTY) {
        row_num = random_number(FIELD_SIDE_LENGTH);
        col_num = random_number(FIELD_SIDE_LENGTH);
    }

    source->cells[row_num][col_num].value = SUDOKU_EMPTY;
}



