#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LOG_FILE "LOG.txt"

void log_event(const char *level, const char *format, ...) {
    time_t current_time = time(NULL);
    struct tm timestamp;

    localtime_r(&current_time, &timestamp);

    char time_text[32];
    strftime(time_text, sizeof(time_text),
             "%Y-%m-%d %H:%M:%S", &timestamp);

    va_list arguments;
    va_start(arguments, format);
    va_list arguments_copy;
    va_copy(arguments_copy, arguments);

    int message_size = vsnprintf(NULL, 0, format, arguments_copy);
    va_end(arguments_copy);

    if (message_size < 0) {
        va_end(arguments);
        return;
    }

    size_t entry_size = (size_t)message_size + strlen(time_text)
                      + strlen(level) + 8;
    char *entry = malloc(entry_size);
    if (entry == NULL) {
        va_end(arguments);
        return;
    }

    int written = snprintf(entry, entry_size, "[%s] [%s] ",
                           time_text, level);
    if (written < 0 || (size_t)written >= entry_size) {
        free(entry);
        va_end(arguments);
        return;
    }

    int message_written = vsnprintf(entry + written,
                                    entry_size - (size_t)written,
                                    format, arguments);
    va_end(arguments);

    if (message_written < 0) {
        free(entry);
        return;
    }

    size_t complete_entry_size = (size_t)written
                               + (size_t)message_written;
    entry[complete_entry_size++] = '\n';
    entry[complete_entry_size] = '\0';

    char temporary_name[] = LOG_FILE ".tmp.XXXXXX";
    int temporary_fd = mkstemp(temporary_name);
    if (temporary_fd < 0) {
        free(entry);
        return;
    }

    FILE *temporary_file = fdopen(temporary_fd, "w");
    if (temporary_file == NULL) {
        close(temporary_fd);
        unlink(temporary_name);
        free(entry);
        return;
    }

    FILE *old_file = fopen(LOG_FILE, "r");
    int success = fputs(entry, temporary_file) != EOF;
    free(entry);

    if (old_file != NULL && success) {
        char buffer[4096];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, 1, sizeof(buffer), old_file)) > 0) {
            if (fwrite(buffer, 1, bytes_read, temporary_file) != bytes_read) {
                success = 0;
                break;
            }
        }

        if (ferror(old_file))
            success = 0;
    }

    if (old_file != NULL)
        fclose(old_file);

    if (fclose(temporary_file) != 0)
        success = 0;

    if (success && rename(temporary_name, LOG_FILE) == 0)
        return;

    unlink(temporary_name);
}

void log_info(const char *message) {
    log_event("INFO", "%s", message);
}