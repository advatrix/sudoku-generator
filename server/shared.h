//
// Created by computer on 12/11/22.
//

#ifndef SERVER_SHARED_H
#define SERVER_SHARED_H

#include "grid.h"

typedef struct shared {
    int semid;
    sudoku_field_t field;
} shared_t;

sudoku_field_t (*swap_func[3])(sudoku_field_t*) = { transpose, swap_rows_small, swap_columns_small };

#endif //SERVER_SHARED_H
