#include "server.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <linux/limits.h>
#include <strings.h>

#define PORT 2000
#define BUFFER_SIZE 100

int start_server(void) {
    int server_fd = -1;
    int client_fd = -1;
    int opt = 1;
    char receive_buffer[BUFFER_SIZE];
    char send_buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        perror("inet_pton");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d...\n", PORT);

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Client connected successfully.\n");

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = client_fd > STDIN_FILENO ? client_fd : STDIN_FILENO;

        struct timeval timeout = {
            .tv_sec = 60,
            .tv_usec = 0
        };

        int result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (result == 0) {
            printf("Timeout reached.\n");
            break;
        }

        if (result < 0) {
            if (errno == EINTR)
                continue;

            perror("select");
            break;
        }

        if (FD_ISSET(client_fd, &read_fds)) {
            ssize_t count = recv(client_fd, receive_buffer,
                                 sizeof(receive_buffer) - 1, 0);

            if (count <= 0) {
                printf("Client disconnected.\n");
                break;
            }

            receive_buffer[count] = '\0';
            receive_buffer[strcspn(receive_buffer, "\r\n")] = '\0';

            printf("[client] %s\n", receive_buffer);

            command_respond(client_fd, receive_buffer);

            if (strcmp(receive_buffer, "exit") == 0)
                break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL)
                break;

            send_buffer[strcspn(send_buffer, "\r\n")] = '\0';

            if (strcmp(send_buffer, "exit") == 0) {
                send(client_fd, "exit", 4, 0);
                break;
            }

            send(client_fd, send_buffer, strlen(send_buffer), 0);
        }
    }

    close(client_fd);
    close(server_fd);

    printf("Server shut down cleanly.\n");
    return EXIT_SUCCESS;
}

void command_respond(int client_fd, const char *command) {
    char operation[16];
    char filename[PATH_MAX];
    unsigned long long file_size;

    int fields = sscanf(command, "%15s %4095s %llu",
                        operation, filename, &file_size);

    if (fields == 3 && strcasecmp(operation, "upload") == 0) {
        if (send(client_fd, "READY\n", 6, MSG_NOSIGNAL) < 0) {
            perror("send");
            return;
        }

        receive_file(client_fd, filename, file_size);
        return;
    }

    fprintf(stderr, "Unknown or invalid command: %s\n", command);
}

void receive_file(int client_fd, const char *filename,
                  unsigned long long file_size) {
    const char *base_name = strrchr(filename, '/');
    base_name = base_name ? base_name + 1 : filename;

    if (*base_name == '\0') {
        fprintf(stderr, "Invalid filename.\n");
        return;
    }

    struct stat info;

    if (stat("./Server", &info) == 0) {
        if (!S_ISDIR(info.st_mode)) {
            fprintf(stderr, "./Server is not a directory.\n");
            return;
        }
    } else if (errno == ENOENT) {
        if (mkdir("./Server", 0777) < 0) {
            perror("mkdir");
            return;
        }
    } else {
        perror("stat");
        return;
    }

    char destination[PATH_MAX];

    int length = snprintf(destination, sizeof(destination),
                          "./Server/%s", base_name);

    if (length < 0 || (size_t)length >= sizeof(destination)) {
        fprintf(stderr, "Destination path is too long.\n");
        return;
    }

    FILE *file = fopen(destination, "wb");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    char buffer[4096];
    unsigned long long total_received = 0;

    while (total_received < file_size) {
        unsigned long long remaining = file_size - total_received;
        size_t requested = remaining < sizeof(buffer)
                         ? (size_t)remaining
                         : sizeof(buffer);

        ssize_t received = recv(client_fd, buffer, requested, 0);

        if (received <= 0) {
            perror("recv");
            fclose(file);
            return;
        }

        fwrite(buffer, 1, (size_t)received, file);
        total_received += (unsigned long long)received;
    }

    fclose(file);
    printf("Received %llu bytes as %s\n", total_received, destination);
}