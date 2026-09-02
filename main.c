#include <stdio.h>
#include <string.h>

// Function to execute a command
void execute_command(const char *command) {
    // Placeholder for command execution logic
    printf("Command '%s' executed successfully.\n", command);
}

// Function to check if the command is valid and execute it
void check_command(const char *command, char commandList[6][256]) {
    for (int i = 0; i < 6; i++) {
        if (strcmp(command, commandList[i]) == 0) {
            execute_command(command);
            return;
        }
    }
    printf("Invalid command: '%s'. Type 'help' for a list of commands.\n", command);
}

// Main function to run the CLI
int main() {
    char commandList[6][256] = {
        "help",
        "list",
        "add",
        "remove",
        "update",
        "exit"
    };
    char command[256];
    printf("ServerVault CLI\n");
    printf("Enter a commmand to execute:\n");
    fgets(command, sizeof(command), stdin);
    command[strcspn(command, "\n")] = 0;
    if (strcmp(command, "exit") == 0) {
        printf("Exiting ServerVault CLI.\n");
        return 0;
    }
    check_command(command, commandList);
}