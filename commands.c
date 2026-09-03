#include "commands.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

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

void move_file(const char *source, const char *destination) {
    if (rename(source, destination) != 0) {
        fprintf(stderr, "Could not move file '%s' to '%s': %s\n",
                source, destination, strerror(errno));
        return;
    }

    printf("File moved successfully.\n");
}

void copy_file(const char *source, const char *destination) {
    FILE *src = fopen(source, "rb");
    if (src == NULL) {
        fprintf(stderr, "Could not open source file '%s': %s\n", source, strerror(errno));
        return;
    }

    FILE *dest = fopen(destination, "wb");
    if (dest == NULL) {
        fprintf(stderr, "Could not open destination file '%s': %s\n", destination, strerror(errno));
        fclose(src);
        return;
    }

    char buffer[8192];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }

    fclose(src);
    fclose(dest);

    printf("File copied successfully.\n");
}

void list_files(void) {
    system("ls -l");
}

void change_directory(const char *path) {
    if (chdir(path) != 0) {
        fprintf(stderr, "Could not change directory to '%s': %s\n", path, strerror(errno));
        return;
    }

    printf("Changed directory to %s\n", path);
}