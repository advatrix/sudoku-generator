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

#define BACKLOG 5
#define CONFIGFILE "sudoku.conf"
#define MAXNAME 255
#define LOG_MAX_MSG_SIZE 255
#define MAX_SHUFFLES_NUM 50
#define MIN_REMOVALS_NUM 40
#define MAX_REMOVALS_NUM 60

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

sudoku_field_t (*shuffle_func[3])(sudoku_field_t*) = {transpose, swap_rows_small, swap_columns_small };

char config_name[MAXNAME+1], log_name[MAXNAME+1], server_name[MAXNAME+1];
int sockfd, logfd, port, shmid, semid;

void logger(const char *message);

void append_time_to_log_name() {
    char nowstr[40];
    struct tm *timenow;

    time_t now = time(NULL);
    timenow = gmtime(&now);

    strftime(nowstr, sizeof(nowstr), "_%Y-%m-%d_%H-%M-%S", timenow);
    strcat(log_name, nowstr);
}

void log_open() {
    if ((logfd = open(log_name, O_WRONLY | O_CREAT, 0666)) == -1) {
        syslog(LOG_INFO, "%s %s: %s", "Cannot open log file", log_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void socket_start() {
    // accept client connections
    struct sockaddr_in servaddr;

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
}

void sighup_lock() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
}

void sighup_unlock() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

void log_lock() {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(logfd, F_SETLKW, &lock) == -1) {
        syslog(LOG_INFO, "Failed to lock log: %s", strerror(errno));
    }
}

void log_unlock() {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(logfd, F_SETLKW, &lock) == -1) {
        syslog(LOG_INFO, "Failed to lock log: %s", strerror(errno));
    }
}

void logger(const char *message) {
    log_lock();
    write(logfd, message, strlen(message));
    write(logfd, "\n", 1);
    log_unlock();
}

void logger_err(const char *message) {
    log_lock();
    write(logfd, message, strlen(message));
    write(logfd, ": ", 2);
    write(logfd, strerror(errno), strlen(strerror(errno)));
    write(logfd, "\n", 1);
    log_unlock();
}

void term() {
    logger("Finishing, waiting for children to exit...");
    wait(NULL); // wait for all children to exit
    logger("Detaching shared memory...");
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        logger_err("Failed to remove shared memory");
    }

    if (shmdt(shared) == -1) {
        logger_err("Failed to detach shared memory");
    }
    logger("Removing semaphore...");
    if (semctl(semid, 1, IPC_RMID) == -1) {
        logger_err("Failed to remove semaphore");
    }
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
    config[st.st_size] = '\0';
    close(f);

    char *conf_start = config;
    int new_port = to_long(strsep(&config, "\n"));
    log_name[0] = '\0';
    strcpy(log_name, strsep(&config, "\n"));
    // add current timestamp to log file name
    append_time_to_log_name();
    close(logfd);
    log_open();
    logger("Master restarted logging");

    if (new_port != port) {
        logger("Port changed");
        port = new_port;
        logger("Set new port");
        syslog(LOG_INFO, "Started listening on %d", port);
        if (close(sockfd) == -1) {
            logger_err("Failed to close old socket");
        };
        logger("Recreating socket for new port...");
        socket_start();
    }

    free(conf_start);
    close(f);
    logger("Reconfigured.");
}

void shared_lock(){
    if (semop(shared->semid, sop_lock, 2) == -1) {
        logger("Unable to lock shared memory");
        exit(1);
    }
    logger("Locked shared memory");
}

void shared_unlock() {
    if (semop(shared->semid, sop_unlock, 1) == -1) {
        logger("Unable to unlock shared memory");
        exit(1);
    }
    logger("Unlocked shared memory");
}

void worker_shuffler(int func_num) {
    close(sockfd);
    shared_lock();
    shared->field = shuffle_func[func_num](&shared->field);
    shared_unlock();
    close(logfd);
    exit(0);
}

void worker_remover() {
    close(sockfd);
    shared_lock();
    remove_cell(&shared->field);
    shared_unlock();
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

    shared_lock();
    shared->field = dummy_field();
    base_grid(&shared->field);
    shared_unlock();

    logger("Start generating field");

    int shuffles_count = random_number(MAX_SHUFFLES_NUM);
    for (int i = 0; i < shuffles_count; ++i) {
        int func_num = random_number(3);
        if (fork() == 0) {
            worker_shuffler(func_num);
        }
    }
    while (wait(NULL) > 0) ;
    sudoku_field_t solution;
    memcpy(&solution, &(shared->field), sizeof(sudoku_field_t));
    logger("Shuffling finished");

    int removals_count = random_number(MAX_REMOVALS_NUM - MIN_REMOVALS_NUM) + MIN_REMOVALS_NUM;
    for (int i = 0; i < removals_count; ++i)
        if (fork() == 0)
            worker_remover();
    while (wait(NULL) > 0) ;
    sudoku_field_t puzzle;
    memcpy(&puzzle, &(shared->field), sizeof(sudoku_field_t));
    logger("Removal finished");

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

    log_open();
    logger("Master started logging");

    key_t ipc_key = ftok(server_name, IPC_PRIVATE);
    logger("Got IPC KEY for shared memory");

    if ((semid = semget(ipc_key, 1, IPC_CREAT | 0666)) == -1) {
        logger_err("Failed to get semaphore ID");
    }
    logger("Got semaphore ID");
    if ((shmid = shmget(ipc_key, sizeof(shared_t), IPC_CREAT | 0666)) == -1) {
        logger_err("Unable to get shared memory ID");
    }
    logger("Got shared memory ID");
    if ((shared = shmat(shmid, NULL, SHM_RND)) == (void *)-1) {
        logger_err("Failed to attach shared memory");
    }
    logger("Attached shared memory with id");
    char buffer[10];
    snprintf(buffer, 10, "%d", shmid);
    logger(buffer);
    shared->semid = semid;
    struct sockaddr_in clientaddr;
    unsigned int client_len;
    socket_start();
    int connfd;
    for (;;) {
        if ((connfd = accept(sockfd, (struct sockaddr*)&clientaddr, &client_len)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        sighup_lock();
        char accepted_message[255];
        strcpy(accepted_message, "Accepted request from ");
        strcat(accepted_message, inet_ntoa(clientaddr.sin_addr));
        logger(accepted_message);

        handle_request(connfd);
        sighup_unlock();
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
    strcpy(log_name, strsep(&config, "\n"));

    // add current timestamp to log file name
    append_time_to_log_name();
    printf("Config name: %s\n", log_name);

    free(conf_start);

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

    switch(fork()) {
        case -1:
            perror("fork");
            exit(1);
        case 0:
            master();
    }

    return 0;
}
