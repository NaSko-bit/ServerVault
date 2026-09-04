#ifndef SERVER_H
#define SERVER_H

int start_server(void);
void command_respond(int client_fd, const char *command);

#endif