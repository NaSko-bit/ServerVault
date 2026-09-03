#include "commands.h"
#include <stdio.h>

void read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s for reading.\n", filename);
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    fclose(file);
}

void add_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Could not create file %s.\n", filename);
        return;
    }

    printf("File %s created successfully.\n", filename);
    fclose(file);
}

void remove_file(const char *filename) {
    if (remove(filename) == 0) {
        printf("File %s removed successfully.\n", filename);
    } else {
        printf("Error: Could not remove file %s.\n", filename);
    }
}


void update_file(const char *filename) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        printf("Error: Could not open file %s for updating.\n", filename);
        return;
    }

    char new_content[8000000];
    if (fgets(new_content, sizeof(new_content), stdin) == NULL) {
        printf("Error: Failed to read new content.\n");
        fclose(file);
        return;
    }
    fputs(new_content, file);
    printf("File %s updated successfully.\n", filename);

    fclose(file);
}