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

static ssize_t receive_line(int client_fd, char *buffer, size_t size) {
    size_t position = 0;

    while (position + 1 < size) {
        char character;
        ssize_t received = recv(client_fd, &character, 1, 0);

        if (received <= 0)
            return received;

        if (character == '\n') {
            buffer[position] = '\0';
            return (ssize_t)position;
        }

        if (character != '\r')
            buffer[position++] = character;
    }

    buffer[size - 1] = '\0';
    return -1;
}

static int send_message(int client_fd, const char *message) {
    size_t length = strlen(message);
    size_t sent_total = 0;

    while (sent_total < length) {
        ssize_t sent = send(client_fd, message + sent_total,
                            length - sent_total, MSG_NOSIGNAL);

        if (sent <= 0)
            return -1;

        sent_total += (size_t)sent;
    }

    return 0;
}

int start_server(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int option = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &option, sizeof(option));

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d...\n", PORT);

    int server_running = 1;

    while (server_running) {
        fd_set waiting_fds;
        FD_ZERO(&waiting_fds);
        FD_SET(server_fd, &waiting_fds);
        FD_SET(STDIN_FILENO, &waiting_fds);

        int result = select(server_fd + 1, &waiting_fds,
                            NULL, NULL, NULL);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &waiting_fds)) {
            char input[BUFFER_SIZE];

            if (fgets(input, sizeof(input), stdin) == NULL)
                break;

            input[strcspn(input, "\r\n")] = '\0';

            if (strcmp(input, "exit") == 0) {
                printf("Shutting down server.\n");
                break;
            }
        }

        if (!FD_ISSET(server_fd, &waiting_fds))
            continue;

        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {
            if (errno == EINTR)
                continue;

            perror("accept");
            break;
        }

        printf("Client connected successfully.\n");

        int client_connected = 1;

        while (client_connected && server_running) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(client_fd, &read_fds);
            FD_SET(STDIN_FILENO, &read_fds);

            int max_fd = client_fd > STDIN_FILENO
                       ? client_fd
                       : STDIN_FILENO;

            result = select(max_fd + 1, &read_fds,
                            NULL, NULL, NULL);

            if (result < 0) {
                if (errno == EINTR)
                    continue;

                perror("select");
                break;
            }

            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                char input[BUFFER_SIZE];

                if (fgets(input, sizeof(input), stdin) == NULL) {
                    server_running = 0;
                    break;
                }

                input[strcspn(input, "\r\n")] = '\0';

                if (strcmp(input, "exit") == 0) {
                    send_message(client_fd, "exit\n");
                    server_running = 0;
                    break;
                }

                if (send_message(client_fd, input) < 0) {
                    perror("send");
                    client_connected = 0;
                }
            }

            if (FD_ISSET(client_fd, &read_fds)) {
                char receive_buffer[BUFFER_SIZE];

                ssize_t count = recv(client_fd, receive_buffer,
                                     sizeof(receive_buffer) - 1, 0);

                if (count == 0) {
                    printf("Client disconnected.\n");
                    client_connected = 0;
                    continue;
                }

                if (count < 0) {
                    perror("recv");
                    client_connected = 0;
                    continue;
                }

                receive_buffer[count] = '\0';
                receive_buffer[strcspn(receive_buffer, "\r\n")] = '\0';

                if (strcmp(receive_buffer, "exit") == 0) {
                    printf("Client requested disconnect.\n");
                    client_connected = 0;
                    continue;
                }

                command_respond(client_fd, receive_buffer);
            }
        }

        close(client_fd);
        printf("Client connection closed.\n");
    }

    close(server_fd);
    printf("Server shut down cleanly.\n");

    return EXIT_SUCCESS;
}

void command_respond(int client_fd, const char *command) {
    char operation[16];
    unsigned int file_count;

    if (sscanf(command, "%15s %u", operation, &file_count) == 2 &&
        strcasecmp(operation, "sync") == 0) {
        sync_files(client_fd, file_count);
        return;
    }

    fprintf(stderr, "Unknown or invalid command: %s\n", command);
}

int receive_file(int client_fd, const char *filename,
                 unsigned long long file_size) {
    const char *base_name = strrchr(filename, '/');
    base_name = base_name ? base_name + 1 : filename;

    if (*base_name == '\0') {
        fprintf(stderr, "Invalid filename.\n");
        return -1;
    }

    struct stat info;

    if (stat("./Server", &info) == 0) {
        if (!S_ISDIR(info.st_mode)) {
            fprintf(stderr, "./Server is not a directory.\n");
            return -1;
        }
    } else if (errno == ENOENT) {
        if (mkdir("./Server", 0777) < 0) {
            perror("mkdir");
            return -1;
        }
    } else {
        perror("stat");
        return -1;
    }

    char destination[PATH_MAX];

    int length = snprintf(destination, sizeof(destination),
                          "./Server/%s", base_name);

    if (length < 0 || (size_t)length >= sizeof(destination)) {
        fprintf(stderr, "Destination path is too long.\n");
        return -1;
    }

    FILE *file = fopen(destination, "wb");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    char buffer[4096];
    unsigned long long total_received = 0;

    while (total_received < file_size) {
        unsigned long long remaining = file_size - total_received;
        size_t requested = remaining < sizeof(buffer)
                         ? (size_t)remaining
                         : sizeof(buffer);

        ssize_t received = recv(client_fd, buffer, requested, 0);

        if (received == 0) {
            fprintf(stderr, "Client disconnected during file transfer.\n");
            fclose(file);
            return -1;
        }

        if (received < 0) {
            perror("recv");
            fclose(file);
            return -1;
        }

        size_t written = fwrite(buffer, 1, (size_t)received, file);

        if (written != (size_t)received) {
            perror("fwrite");
            fclose(file);
            return -1;
        }

        total_received += (unsigned long long)received;
    }

    fclose(file);

    file_orginize(destination);

    printf("Received and organized: %s\n", destination);
    return 0;
}

void file_orginize(const char *filepath) {
    const char *filename = strrchr(filepath, '/');
    filename = filename != NULL ? filename + 1 : filepath;

    const char *extension = strrchr(filename, '.');

    if (extension == NULL || extension == filename) {
        fprintf(stderr, "Unsupported file type: %s\n", filename);
        return;
    }

    const char *directory = NULL;

    if (strcasecmp(extension, ".mp3") == 0 ||
        strcasecmp(extension, ".wav") == 0 ||
        strcasecmp(extension, ".ogg") == 0 ||
        strcasecmp(extension, ".flac") == 0 ||
        strcasecmp(extension, ".aac") == 0 ||
        strcasecmp(extension, ".m4a") == 0) {
        directory = "audio";
    } else if (strcasecmp(extension, ".png") == 0 ||
               strcasecmp(extension, ".jpg") == 0 ||
               strcasecmp(extension, ".jpeg") == 0 ||
               strcasecmp(extension, ".gif") == 0 ||
               strcasecmp(extension, ".bmp") == 0 ||
               strcasecmp(extension, ".webp") == 0) {
        directory = "image";
    } else if (strcasecmp(extension, ".pdf") == 0) {
        directory = "pdf";
    } else if (strcasecmp(extension, ".txt") == 0 ||
               strcasecmp(extension, ".md") == 0 ||
               strcasecmp(extension, ".csv") == 0 ||
               strcasecmp(extension, ".log") == 0 ||
               strcasecmp(extension, ".json") == 0 ||
               strcasecmp(extension, ".xml") == 0 ||
               strcasecmp(extension, ".doc") == 0 ||
               strcasecmp(extension, ".docx") == 0) {
        directory = "text";
    } else if (strcasecmp(extension, ".mp4") == 0 ||
               strcasecmp(extension, ".mkv") == 0 ||
               strcasecmp(extension, ".avi") == 0 ||
               strcasecmp(extension, ".mov") == 0 ||
               strcasecmp(extension, ".webm") == 0 ||
               strcasecmp(extension, ".mpeg") == 0 ||
               strcasecmp(extension, ".mpg") == 0) {
        directory = "video";
    }

    if (directory == NULL) {
        fprintf(stderr, "Unsupported file type: %s\n", filename);
        return;
    }

    char directory_path[PATH_MAX];

    int length = snprintf(directory_path, sizeof(directory_path),
                          "./Server/%s", directory);

    if (length < 0 || (size_t)length >= sizeof(directory_path)) {
        fprintf(stderr, "Directory path is too long.\n");
        return;
    }

    struct stat info;

    if (stat(directory_path, &info) != 0) {
        if (errno != ENOENT || mkdir(directory_path, 0755) < 0) {
            perror("mkdir");
            return;
        }
    } else if (!S_ISDIR(info.st_mode)) {
        fprintf(stderr, "%s is not a directory.\n", directory_path);
        return;
    }

    char new_path[PATH_MAX];

    length = snprintf(new_path, sizeof(new_path),
                      "%s/%s", directory_path, filename);

    if (length < 0 || (size_t)length >= sizeof(new_path)) {
        fprintf(stderr, "Destination path is too long.\n");
        return;
    }

    if (rename(filepath, new_path) < 0) {
        fprintf(stderr, "Could not organize '%s' to '%s': %s\n",
                filepath, new_path, strerror(errno));
        return;
    }

    printf("Organized file: %s\n", new_path);
}

void sync_directory_files(const char *directory) {
    char command[PATH_MAX + 50];

    int length = snprintf(command, sizeof(command),
                          "rsync -av --ignore-existing ./Server/%s/ ./ServerVault/%s/",
                          directory, directory);

    if (length < 0 || (size_t)length >= sizeof(command)) {
        fprintf(stderr, "Command is too long.\n");
        return;
    }

    int result = system(command);

    if (result != 0) {
        fprintf(stderr, "Failed to sync files from '%s' to ServerVault.\n", directory);
    } else {
        printf("Files from '%s' synced to ServerVault successfully.\n", directory);
    }
}

void sync_files(int client_fd, unsigned int file_count) {
    char header[PATH_MAX + 64];

    if (send_message(client_fd, "SYNC_READY\n") < 0)
        return;

    for (unsigned int i = 0; i < file_count; i++) {
        ssize_t length = receive_line(client_fd, header, sizeof(header));

        if (length <= 0) {
            fprintf(stderr, "Client disconnected during synchronization.\n");
            return;
        }

        char filename[PATH_MAX];
        unsigned long long file_size;

        if (sscanf(header, "FILE %4095s %llu",
                   filename, &file_size) != 2) {
            send_message(client_fd, "ERROR Invalid file header\n");
            return;
        }

        if (send_message(client_fd, "READY\n") < 0)
            return;

        if (receive_file(client_fd, filename, file_size) != 0) {
            send_message(client_fd, "ERROR FILE_RECEIVE_FAILED\n");
            return;
        }

        if (send_message(client_fd, "FILE_STORED\n") < 0)
            return;
    }

    ssize_t length = receive_line(client_fd, header, sizeof(header));

    if (length > 0 && strcmp(header, "SYNC_DONE") == 0)
        send_message(client_fd, "SYNC_COMPLETE\n");
}

