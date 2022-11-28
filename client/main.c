#include <stdio.h>
#include "../sudoku_field.h"

int main() {
    sudoku_field_t s = dummy_field();

    print(&s);

    return 0;
}
