#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include "commands.h"
#include <stdlib.h>
#include <sys/select.h>

#define PORT 2000
#define BUFFER_SIZE 100

typedef enum {
    CMD_INVALID,
    CMD_HELP,
    CMD_LIST,
    CMD_ADDFILE,
    CMD_REMOVEFILE,
    CMD_UPDATEFILE,
    CMD_READFILE,
    MOVE_FILE,
    COPY_FILE,
    CHANGE_DIR,
    TCP_START,
    CMD_EXIT
} Command;

Command parse_command(const char *command);
void execute_command(Command command, int argc, char *argv[]);

Command parse_command(const char *command) {
    const char *names[] = {
        [CMD_HELP] = "help",
        [CMD_LIST] = "list",
        [CMD_ADDFILE] = "add_file",
        [CMD_REMOVEFILE] = "remove_file",
        [CMD_UPDATEFILE] = "update_file",
        [CMD_READFILE] = "read_file",
        [MOVE_FILE] = "move_file",
        [COPY_FILE] = "copy_file",
        [CHANGE_DIR] = "change_directory",
        [TCP_START] = "tcp_start",
        [CMD_EXIT] = "exit"
    };

    for (int i = CMD_HELP; i <= CMD_EXIT; i++) {
        if (strcmp(command, names[i]) == 0)
            return i;
    }

    return CMD_INVALID;
}

void execute_command(Command command, int argc, char *argv[]) {
    const char *argument = argc >= 3 ? argv[2] : NULL;

    switch (command) {
        case CMD_HELP:
            printf("Available commands:\n");
            printf("add_file <name>\n");
            printf("remove_file <filename>\n");
            printf("update_file <filename>\n");
            printf("read_file <filename>\n");
            printf("move_file <old_filename> <new_filename>\n");
            printf("copy_file <source> <destination>\n");
            printf("change_directory <path>\n");
            printf("list [directory]\n");
            break;

        case CMD_LIST: {
            const char *directory = argc >= 3 ? argv[2] : ".";

            printf("Files in '%s':\n", directory);
            list_files(directory);
            break;
        }

        case CMD_ADDFILE: {
            if (argument == NULL) {
                printf("Usage: add_file <name>\n");
                break;
            }

            char full_name[512];
            snprintf(full_name, sizeof(full_name), "%s.txt", argument);
            add_file(full_name);
            break;
        }

        case CMD_REMOVEFILE:
            if (argument == NULL) {
                printf("Usage: remove_file <filename>\n");
                break;
            }
            remove_file(argument);
            break;

        case CMD_UPDATEFILE:
            if (argument == NULL) {
                printf("Usage: update_file <filename>\n");
                break;
            }
            update_file(argument);
            break;

        case CMD_READFILE:
            if (argument == NULL) {
                printf("Usage: read_file <filename>\n");
                break;
            }
            read_file(argument);
            break;

        case MOVE_FILE:
            if (argc < 4) {
                printf("Usage: move_file <old_filename> <new_filename>\n");
                break;
            }
            move_file(argv[2], argv[3]);
            break;
        
        case COPY_FILE:
            if (argc < 4) {
                printf("Usage: copy_file <source> <destination>\n");
                break;
            }
            copy_file(argv[2], argv[3]);
            break;

        case CHANGE_DIR:
            if (argument == NULL) {
                printf("Usage: change_directory <path>\n");
                break;
            }
            change_directory(argument);
            break;

        case TCP_START:
            tcp_start();
            break;

        default:
            printf("Invalid command.\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2 || parse_command(argv[1]) != TCP_START) {
        printf("Usage: %s tcp_start\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Starting TCP server...\n");

    //SERVER SETUP
    struct sockaddr_in client, server;
    int lfd, confd;
    socklen_t n;
    char r_buff[BUFFER_SIZE], s_buff[BUFFER_SIZE];

    int opt = 1;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(lfd);
        return EXIT_FAILURE;
    }

    // 2. Configure server address safely using htons
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT); 
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 3. Bind with error checking
    if (bind(lfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(lfd);
        return EXIT_FAILURE;
    }

    // 4. Listen for incoming connections
    if (listen(lfd, 1) < 0) {
        perror("Listen failed");
        close(lfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // 5. Accept client connection
    n = sizeof(client);
    confd = accept(lfd, (struct sockaddr *)&client, &n);
    if (confd < 0) {
        perror("Accept failed");
        close(lfd);
        exit(EXIT_FAILURE);
    }
    printf("Client connected successfully.\n");

    // 6. Safe communication loop
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(confd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = confd > STDIN_FILENO ? confd : STDIN_FILENO;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            break;
        }

        if (FD_ISSET(confd, &read_fds)) {
            int valread = recv(confd, r_buff, sizeof(r_buff) - 1, 0);

            if (valread <= 0) {
                printf("Client disconnected.\n");
                break;
            }

            r_buff[valread] = '\0';
            printf("[client] %s\n", r_buff);

            if (strcmp(r_buff, "exit") == 0)
                break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(s_buff, sizeof(s_buff), stdin) == NULL)
                break;

            s_buff[strcspn(s_buff, "\r\n")] = '\0';

            if (strcmp(s_buff, "exit") == 0) {
                send(confd, s_buff, strlen(s_buff), 0);
                break;
            }

            send(confd, s_buff, strlen(s_buff), 0);
        }
    }

    // 7. Cleanup
    close(confd);
    close(lfd);
    printf("Server shut down cleanly.\n");

    return 0;



    /*if (argc < 2) {
        printf("Usage: %s <command> [argument]\n", argv[0]);
        return 1;
    }

    Command command = parse_command(argv[1]);

    if (command == CMD_EXIT) {
        printf("Exiting ServerVault.\n");
        return 0;
    }

    execute_command(command, argc, argv);
    */
}