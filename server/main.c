#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "grid.h"
#include "random_dev.h"
#include "../utils.h"

#define PORT 9001
#define BACKLOG 5
#define COMMAND_LENGTH 9
#define CONFIGFILE "sudoku.conf"
#define LOGFILE "sudoku.log"
#define MAXNAME 255
#define LOG_MAX_MSG_SIZE 255
#define SHARED_MEMORY_SIZE 1024
#define MAX_SWAPS_NUM 50

// todo привязать разделяемую память к глобальной переменной чтобы удалять её в term

typedef struct shared {
    int semid;
    sudoku_field_t field;
} shared_t;

shared_t *shared;

struct sembuf sop_lock[2] = {
        0, 0, 0,
        0, 1, 0
};

struct sembuf sop_unlock[1] = {
        0, -1, 0
};

sudoku_field_t (*swap_func[3])(sudoku_field_t*) = { transpose, swap_rows_small, swap_columns_small };

char config_name[MAXNAME+1], log_name[MAXNAME+1], server_name[MAXNAME+1];
int sockfd, logfd, port;

void append_time_to_log_name() {
    char nowstr[40];
    struct tm *timenow;

    time_t now = time(NULL);
    timenow = gmtime(&now);

    strftime(nowstr, sizeof(nowstr), "_%Y-%m-%d_%H-%M-%S", timenow);
    strcat(log_name, nowstr);
}

void logger(const char *message) {
    write(logfd, message, strlen(message));
    write(logfd, "\n", 1);
}

void term() {
    logger("Finishing, waiting for children to exit...");
    wait(NULL); // wait for all children to exit
    logger("Closing socket...");
    close(sockfd);
    logger("Closing log...");
    close(logfd);
    exit(0);
}

void hup() {
    logger("Reconfiguring...");
    int f;
    struct stat st;
    if ((f = open(config_name, O_RDONLY)) == -1) {
        syslog(LOG_INFO, "%s: %s", config_name, strerror(errno));
        logger("Cannot open new config file.");
        term();
    }
    if (fstat(f, &st) == -1) { // если не удалось считать размер файла
        syslog(LOG_INFO, "%s: %s", config_name, strerror(errno));
        logger("Cannot retrieve config file stats.");
        term();
    }

    char *config = NULL;
    if ((config = malloc(st.st_size + 1)) == NULL) {
        perror("malloc");
        exit(1);
    }
    read(f, config, st.st_size);
    close(f);

    char *conf_start = config;
    port = to_long(strsep(&config, "\n"));
    log_name[0] = '\0';
    strcpy(log_name, config);
    // add current timestamp to log file name
    append_time_to_log_name();
    free(conf_start);

    close(f);
}

void worker_swapper(int func_num) {
    close(sockfd);

    if (semop(shared->semid, sop_lock, 2) == -1) {
        logger("Unable to lock shared memory");
        exit(1);
    }

    logger("Locked shared memory");
    shared->field = swap_func[func_num](&shared->field);

    if (semop(shared->semid, sop_unlock, 1) == -1) {
        logger("Unable to unlock shared memory");
        exit(1);
    }
    logger("Unlocked shared memory");

    close(logfd);
    exit(0);
}

void handle_request(int connfd) {
    const char *expected_command = "generate";
    char *response;
    char command[strlen(expected_command) + 1];
    memset(command, 0, strlen(expected_command) + 1);
    if (read(connfd, command, strlen(expected_command)) == -1) {
        logger("Could not read from socket");
        return;
    }
    logger("Read from socket");

    if (strcmp(command, expected_command) != 0) {
        char message[LOG_MAX_MSG_SIZE];
        strcpy(message, "Unknown command: ");
        strcat(message, command);
        logger(message);

        response = "Список доступных команд: 1. generate";
        u_short status = strlen(response) + 1;
        write(connfd, &status, sizeof(u_short));
        write(connfd, response, strlen(response) + 1);
        return;
    }

    key_t ipc_key = ftok(server_name, IPC_PRIVATE);
    logger("Got IPC KEY for shared memory");

    int semid = semget(ipc_key, 1, IPC_CREAT | 0666);
    logger("Got semaphore ID");

    int shmid;
    if ((shmid = shmget(ipc_key, sizeof(shared_t), IPC_CREAT | 0666)) == -1) {
        logger("Unable to get shared memory ID");
        logger(strerror(errno));
    };
    logger("Got shared memory ID");
    shared = shmat(shmid, NULL, SHM_RND);
    logger("Attached shared memory with id");
    char buffer[10];
    snprintf(buffer, 10, "%d", shmid);
    logger(buffer);
    shared->semid = semid;
    shared->field = dummy_field();
    base_grid(&shared->field);

    logger("Start generating field");

    int swaps = random_number(MAX_SWAPS_NUM);

    for (int i = 0; i < swaps; ++i) {
        int func_num = random_number(3);
        if (fork() == 0) {
            worker_swapper(func_num);
        }
    }

    while (wait(NULL) > 0) ;

    logger("wait");

//    sudoku_field_t sudoku_field = dummy_field();
//    base_grid(&sudoku_field);
//    sudoku_field_t transposed = transpose(&sudoku_field);
//
//    sudoku_field_t swapped = swap_rows_small(&transposed);
//
//    sudoku_field_t swapped_again = swap_columns_small(&swapped);
//
//    for (int i = 0; i < 60; ++i) {
//        remove_cell(&swapped_again);
//    }

    sudoku_field_t solution;
    memcpy(&solution, &(shared->field), sizeof(sudoku_field_t));

    for (int i = 0; i < 60; ++i)
        remove_cell(&shared->field);

    sudoku_field_t puzzle;
    memcpy(&puzzle, &(shared->field), sizeof(sudoku_field_t));

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        logger("Failed to remove shared memory");
        logger(strerror(errno));
    }

    if (shmdt(shared) == -1) {
        logger("Failed to detach shared memory");
        logger(strerror(errno));
    }

    logger("Removed shared memory");
    logger("Generated sudoku");
    // send data to client
    u_short status = RESPONSE_OK;
    write(connfd, &status, sizeof(u_short));
    write(connfd, &puzzle, sizeof(puzzle));
    write(connfd, &solution, sizeof(solution));
    logger("Sent to client");
    close(connfd);
    logger("Connection closed");
}

void master() {
    setsid();
    openlog("sudoku_master", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "%s", "Started");
    if ((logfd = open(log_name, O_WRONLY | O_CREAT, 0666)) == -1) {
        syslog(LOG_INFO, "%s %s: %s", "Cannot open log file", log_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    logger("Master started logging");


    // accept client connections
    int connfd;
    unsigned int client_len;
    struct sockaddr_in servaddr, clientaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        logger("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    logger("Created socket");

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr))) {
        logger("Socket bind failed");
        logger(strerror(errno));
        exit(EXIT_FAILURE);
    }
    logger("Bind socket");

    if (listen(sockfd, BACKLOG)) {
        logger("Listen failed");
        exit(EXIT_FAILURE);
    }
    logger("Listen...");

    for (;;) {
        if ((connfd = accept(sockfd, (struct sockaddr*)&clientaddr, &client_len)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        char accepted_message[255];
        strcpy(accepted_message, "Accepted request from ");
        strcat(accepted_message, inet_ntoa(clientaddr.sin_addr));
        logger(accepted_message);

        handle_request(connfd);
    }

}

int main(int argc, char *argv[], char *envp[]) {
    char *p;
    struct stat st;
    struct sigaction act;
    int configfd;

    // configuration
    if (argc > 2) {
        fprintf(stderr, "Usage: %s ( configfile )\n", argv[0]);
        exit(1);
    }
    if (argc > 1) {
        if (strlen(argv[1]) > MAXNAME) {
            fprintf(stderr, "Name '%s' of configfile is too long\n", argv[1]);
            exit(1);
        }
        strcpy(config_name, argv[1]);
    }
    else if ((p = getenv("CONFIG")) != NULL) {
        if (strlen(p) > MAXNAME) {
            fprintf(stderr, "Name '%s' of configfile is too long\n", p);
            exit(1);
        }
        strcpy(config_name, p);
    }
    else
        strcpy(config_name, CONFIGFILE);

    strcpy(server_name, argv[0]);

    if ((configfd = open(config_name, O_RDONLY)) == -1) {
        perror(config_name);
        exit(1);
    }
    if (fstat(configfd, &st) == -1) {
        perror(config_name);
        exit(1);
    }

    printf("Opened config file\n");

    char *config = NULL;
    if ((config = malloc(st.st_size + 1)) == NULL) {
        perror("malloc");
        exit(1);
    }
    read(configfd, config, st.st_size);
    config[st.st_size] = '\0';
    close(configfd);

    printf("Read config, close file\n");

    char *conf_start = config;
    port = to_long(strsep(&config, "\n"));
    strcpy(log_name, config);

    printf("Config name: %s\n", log_name);

    // add current timestamp to log file name
    append_time_to_log_name();
    printf("Config name: %s\n", log_name);

    free(conf_start);
    printf("Config name: %s\n", log_name);

    printf("Becoming a daemon...\n");

    // becoming a daemon
    act.sa_handler = hup;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &act, NULL);
    act.sa_handler = term;
    sigaction(SIGTERM, &act, NULL);

    for (int f = 0; f < 256; f++) close(f);

    // sudo needed
    // chdir("/");


    // todo remove to become a true daemon
//    switch(fork()) {
//        case -1:
//            perror("fork");
//            exit(1);
//        case 0:
//            master();
//    }
    master();

    return 0;
}
