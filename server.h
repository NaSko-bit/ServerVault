#ifndef SERVER_H
#define SERVER_H

int start_server(void);
void command_respond(int client_fd, const char *command);
void receive_file(int client_fd, const char *filename,
                  unsigned long long file_size);
void file_orginize(const char *filepath);

#endif