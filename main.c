#include <stdio.h>
#include <string.h>
#include "commands.h"
#include "commands.c"

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
void execute_command(Command command);

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

void execute_command(Command command) {
    switch (command) {
        case CMD_HELP:
            printf("Available commands.\n");
            break;
        case CMD_LIST:
            printf("Listing all items...\n");
            break;
        case CMD_ADDFILE:
            printf("Add new file name: ");
            char filename[256];
            if (fgets(filename, sizeof(filename), stdin) == NULL) {
                printf("Failed to read file name.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = '\0';
            char full_name[512];
            snprintf(full_name, sizeof(full_name), "%s.txt", filename);
            add_file(full_name);
            break;
        case CMD_REMOVEFILE:
            printf("enter file name to remove: ");
            char remove_filename[256];
            if (fgets(remove_filename, sizeof(remove_filename), stdin) == NULL) {
                printf("Failed to read file name.\n");
                break;
            }
            remove_filename[strcspn(remove_filename, "\n")] = '\0';
            remove_file(remove_filename);
            break;
        case CMD_UPDATEFILE:
            printf("enter file name to update: ");
            char update_filename[256];
            if (fgets(update_filename, sizeof(update_filename), stdin) == NULL) {
                printf("Failed to read file name.\n"); 
                break;
            };
            update_filename[strcspn(update_filename, "\n")] = '\0';
            update_file(update_filename);
            break;
        case CMD_READFILE:
            printf("enter file name to read: ");
            char read_filename[256];
            if (fgets(read_filename, sizeof(read_filename), stdin) == NULL) {
                printf("Failed to read file name.\n");
                break;
            }
            read_filename[strcspn(read_filename, "\n")] = '\0';
            read_file(read_filename);
            break;
        default:
            printf("Invalid command.\n");
            break;
    }
}

// Main function to run the CLI
int main(void) {

    char input[256];

    while (1) {
        printf("ServerVault CLI\n");
        printf("Enter a command to execute:\n");

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0';

        Command command = parse_command(input);

        if (command == CMD_EXIT) {
            printf("Exiting ServerVault CLI.\n");
            break;
        }

        execute_command(command);
    }

    return 0;
}