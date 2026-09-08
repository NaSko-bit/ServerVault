#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#define LOG_DATABASE "../ServerVaultDB"

static int initialize_database(sqlite3 *database) {
    const char *create_table =
        "CREATE TABLE IF NOT EXISTS LOGS ("
        "LogID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "TImeStamp TEXT NOT NULL, "
        "Source TEXT NOT NULL, "
        "LogInfo TEXT NOT NULL"
        ")";
    char *error_message = NULL;
    int result = sqlite3_exec(database, create_table, NULL, NULL,
                              &error_message);

    if (result != SQLITE_OK) {
        fprintf(stderr, "Could not create log table: %s\n",
                error_message != NULL ? error_message : "unknown error");
        sqlite3_free(error_message);
        return 0;
    }

    return 1;
}

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

    size_t entry_size = (size_t)message_size + 1;
    char *entry = malloc(entry_size);
    if (entry == NULL) {
        va_end(arguments);
        return;
    }

    int message_written = vsnprintf(entry, entry_size, format, arguments);
    va_end(arguments);

    if (message_written < 0) {
        free(entry);
        return;
    }

    sqlite3 *database = NULL;
    if (sqlite3_open(LOG_DATABASE, &database) != SQLITE_OK) {
        fprintf(stderr, "Could not open log database: %s\n",
                database != NULL ? sqlite3_errmsg(database) : "unknown error");
        if (database != NULL)
            sqlite3_close(database);
        free(entry);
        return;
    }

    if (!initialize_database(database)) {
        sqlite3_close(database);
        free(entry);
        return;
    }

    sqlite3_stmt *statement = NULL;
    const char *insert_log =
        "INSERT INTO LOGS (TImeStamp, Source, LogInfo) "
        "VALUES (?, ?, ?)";

    int result = sqlite3_prepare_v2(database, insert_log, -1,
                                    &statement, NULL);
    if (result == SQLITE_OK) {
        sqlite3_bind_text(statement, 1, time_text, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 2, level != NULL ? level : "",
                          -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 3, entry, -1, SQLITE_TRANSIENT);
        result = sqlite3_step(statement);
    }

    if (result != SQLITE_DONE)
        fprintf(stderr, "Could not write log entry: %s\n",
                sqlite3_errmsg(database));

    sqlite3_finalize(statement);
    sqlite3_close(database);
    free(entry);
}

void log_info(const char *message) {
    log_event("INFO", "%s", message);
}