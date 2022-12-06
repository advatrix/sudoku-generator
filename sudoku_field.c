#include "sudoku_field.h"
#include <stdio.h>


void print(sudoku_field_t* sudoku_field) {
    for (int i = 0; i < sudoku_field->side_length; ++i) {
        if (i % REGION_SIDE_LENGTH == 0) printf("|-------|-------|-------|\n");

        for (int j = 0; j < sudoku_field->side_length; ++j) {
            if (j % REGION_SIDE_LENGTH == 0) printf("| ");
            if (sudoku_field->cells[i][j].value == SUDOKU_EMPTY) {
                printf("  ");
            }
            else
                printf("%d ", sudoku_field->cells[i][j].value);
        }

        printf("|\n");
    }
    printf("|-------|-------|-------|\n");
}

sudoku_field_t dummy_field() {
    sudoku_field_t result = {0};
    result.side_length = FIELD_SIDE_LENGTH;
    for (int i = 0; i < result.side_length; ++i) {
        for (int j = 0; j < result.side_length; ++j) {
            result.cells[i][j].value = 0;
        }
    }
    return result;
}
