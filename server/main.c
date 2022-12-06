#include <stdio.h>

#include "grid.h"

int main() {
    printf("Hello, World!\n");

    sudoku_field_t sudoku_field = dummy_field();
    base_grid(&sudoku_field);

    print(&sudoku_field);

    sudoku_field_t transposed = transpose(&sudoku_field);
    print(&transposed);

    sudoku_field_t swapped = swap_rows_small(&transposed);
    print(&swapped);

    sudoku_field_t swapped_again = swap_columns_small(&swapped);
    print(&swapped_again);

    for (int i = 0; i < 40; ++i) {
        remove_cell(&swapped_again);
        printf("%d\n", i);
    }
    print(&swapped_again);

    return 0;
}
