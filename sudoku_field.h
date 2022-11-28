#ifndef SUDOKU_GENERATOR_SUDOKU_FIELD_H
#define SUDOKU_GENERATOR_SUDOKU_FIELD_H

#define REGION_SIDE_LENGTH 3
#define FIELD_SIDE_LENGTH  9

#define SUDOKU_EMPTY       (-1)
#endif //SUDOKU_GENERATOR_SUDOKU_FIELD_H


typedef struct sudoku_cell {
    short value;
} sudoku_cell_t;

typedef struct sudoku_field {
    int side_length;
    sudoku_cell_t cells[FIELD_SIDE_LENGTH][FIELD_SIDE_LENGTH];
} sudoku_field_t;

sudoku_field_t dummy_field();
void print(sudoku_field_t* sudoku_field);
