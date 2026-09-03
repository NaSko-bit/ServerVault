#include <stdio.h>
#include <string.h>
#include "commands.h"

typedef enum {
    CMD_INVALID,
    CMD_HELP,
    CMD_LIST,
    CMD_ADDFILE,
    CMD_REMOVEFILE,
    CMD_UPDATEFILE,
    CMD_READFILE,
    CMD_EXIT
} Command;

Command parse_command(const char *command);
void execute_command(Command command, const char *argument);

Command parse_command(const char *command) {
    const char *names[] = {
        [CMD_HELP] = "help",
        [CMD_LIST] = "list",
        [CMD_ADDFILE] = "add_file",
        [CMD_REMOVEFILE] = "remove_file",
        [CMD_UPDATEFILE] = "update_file",
        [CMD_READFILE] = "read_file",
        [CMD_EXIT] = "exit"
    };

    for (int i = CMD_HELP; i <= CMD_EXIT; i++) {
        if (strcmp(command, names[i]) == 0)
            return i;
    }

    return CMD_INVALID;
}

void execute_command(Command command, const char *argument) {
    switch (command) {
        case CMD_HELP:
            printf("Available commands:\n");
            printf("add_file <name>\n");
            printf("remove_file <filename>\n");
            printf("update_file <filename>\n");
            printf("read_file <filename>\n");
            break;

        case CMD_LIST:
            printf("Listing all items...\n");
            break;

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

        default:
            printf("Invalid command.\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <command> [argument]\n", argv[0]);
        return 1;
    }

    Command command = parse_command(argv[1]);

    if (command == CMD_EXIT) {
        printf("Exiting ServerVault.\n");
        return 0;
    }

    const char *argument = argc >= 3 ? argv[2] : NULL;
    execute_command(command, argument);

    return 0;
}