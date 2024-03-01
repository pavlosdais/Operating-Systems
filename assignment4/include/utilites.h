#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <sys/stat.h>

#include "common.h"

// returns true if the element is a directory, false otherwise
bool is_directory(const struct stat st);

// returns true if the element is a symbolic link, false otherwise
bool is_sym_link(const struct stat st);

// returns true if the element is a hard link, false otherwise
bool is_hard_link(const struct stat st);

// returns true if the element is a regular file, false otherwise
bool is_reg_file(const struct stat st);

// creates a complete path using the path and the name
char *make_path(const char *path, char *name);

// removes a string that is contained in another string and returns the ouput
char *str_remove(const char *str, const char *sub);

// prints the file statistics of a file
void print_file_stats(struct stat file_stat, const char *file_name);

// create file_info struct with the given path and inode number
// needed for the hash table
file_info *create_finfo(char *path, ino_t inode);

// destroy memory used by file_info
void destroy_finfo(file_info *finfo);

// compares two time specs (last modification date)
int compare_timespec(struct timespec t1, struct timespec t2);

// wrapper for realpath
char* get_path(const char *path);

// returns the relative path
char *get_relative_path(char *dir, const char *relative_path);

// returns true if the path is a full path, otherwise false
bool is_full_path(const char *path);

// removes the last string before "/" from the path
void remove_last_str(char *path);

// returns the target of a link
char* get_link_target(const char *path);

// return the full path of a target
char *get_target_full_path(const char *path, const char *src);

// returns the full path of a link
char *get_link_full_path(const char *path);

#endif // UTILITIES_H
