#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

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
            .tv_sec = 30,
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