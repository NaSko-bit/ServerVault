#ifndef LOG_H
#define LOG_H

void log_info(const char *message);
void log_event(const char *level, const char *format, ...);

#endif