#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>

#include "grid.h"
#include "random_dev.h"

#define PORT 9001
#define BACKLOG 5

int main() {
    // accept client connections
    int sockfd, connfd, client_len;
    struct sockaddr_in servaddr, clientaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr))) {
        fprintf(stderr, "bind failed\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((connfd = accept(sockfd, (struct sockaddr*)&clientaddr, &client_len)) == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }


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




    for (int i = 0; i < 60; ++i) {
        remove_cell(&swapped_again);
        printf("%d\n", i);
    }
    print(&swapped_again);


    // send data to client
    write(connfd, &swapped_again, sizeof(swapped_again));

    close(connfd);
    close(sockfd);

    return 0;
}
