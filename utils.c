//
// Created by computer on 12/11/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

long to_long(const char *str) {
    char *ptr;
    long result = strtol(str, &ptr, 0);
    if (strlen(str) != ptr - str) {
        fprintf(stderr, "failed to convert str to long: %s\n", str);
        exit(1);
    }
    return result;
}
