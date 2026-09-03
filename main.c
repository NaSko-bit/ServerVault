#include <stdio.h>
#include <string.h>

typedef enum {
    CMD_INVALID,
    CMD_HELP,
    CMD_LIST,
    CMD_ADD,
    CMD_REMOVE,
    CMD_UPDATE,
    CMD_EXIT
} Command;

Command parse_command(const char *command);
void execute_command(Command command);

Command parse_command(const char *command) {
    const char *names[] = {
        [CMD_HELP] = "help",
        [CMD_LIST] = "list",
        [CMD_ADD] = "add",
        [CMD_REMOVE] = "remove",
        [CMD_UPDATE] = "update",
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
        case CMD_ADD:
            printf("Adding a new item...\n");
            break;
        case CMD_REMOVE:
            printf("Removing an item...\n");
            break;
        case CMD_UPDATE:
            printf("Updating an existing item...\n");
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