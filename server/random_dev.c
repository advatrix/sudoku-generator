//
// Created by rakhmerov on 06.12.2022.
//

#include "random_dev.h"

int random_number(int n) {
    return rand() % n;
}

void seed() {
    srand(time(NULL));
}