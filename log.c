#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_FILE "LOG.txt"

void log_event(const char *level, const char *format, ...) {
    FILE *file = fopen(LOG_FILE, "a");
    if (file == NULL)
        return;

    time_t current_time = time(NULL);
    struct tm timestamp;

    localtime_r(&current_time, &timestamp);

    char time_text[32];
    strftime(time_text, sizeof(time_text),
             "%Y-%m-%d %H:%M:%S", &timestamp);

    fprintf(file, "[%s] [%s] ", time_text, level);

    va_list arguments;
    va_start(arguments, format);
    vfprintf(file, format, arguments);
    va_end(arguments);

    fputc('\n', file);
    fclose(file);
}

void log_info(const char *message) {
    log_event("INFO", "%s", message);
}