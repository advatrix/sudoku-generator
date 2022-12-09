//
// Created by rakhmerov on 06.12.2022.
//

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "random_dev.h"

u_int8_t random_number(int n) {
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd == -1) {
        fprintf(stderr, "Cannot open /dev/urandom\n");
        return -1;
    }
    u_int8_t random_number;
    if (read(urandom_fd, &random_number, sizeof(u_int8_t)) == -1) {
        fprintf(stderr, "Cannot read from /dev/urandom\n");
        return -1;
    }
    if (close(urandom_fd) == -1) {
        fprintf(stderr, "Cannot close /dev/urandom\n");
        return -1;
    }
    return random_number % n;
}
