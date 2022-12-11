#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../sudoku_field.h"
#include "../utils.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s host port\n", argv[0]);
        exit(1);
    }

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    if (inet_aton(argv[1], &(servaddr.sin_addr)) == 0) {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
    }
    servaddr.sin_port = htons(to_long(argv[2]));

    if (connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr))) {
        fprintf(stderr, "Could not connect to server\n");
        exit(0);
    }

    const char *command = "generate";

    if(write(sockfd, command, strlen(command) + 1) == -1) {
        fprintf(stderr, "could not write to sock\n");
        exit(EXIT_FAILURE);
    }
    u_short response_status;
    read(sockfd, &response_status, sizeof(response_status));
    if (response_status == RESPONSE_OK) {
        sudoku_field_t answer;
        read(sockfd, &answer, sizeof(answer));
        print(&answer);
    } else {
        char response[255];
        read(sockfd, response, response_status);
        printf("%s\n", response);
    }

    close(sockfd);
    return 0;
}
