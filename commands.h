#ifndef COMMANDS_H
#define COMMANDS_H

void read_file(const char *filename);
void add_file(const char *filename);
void remove_file(const char *filename);
void update_file(const char *filename);
void move_file(const char *old_filename, const char *new_filename);
void copy_file(const char *source, const char *destination);
void list_files(void);
void change_directory(const char *path);

#endif