#ifndef COPY_FILE
#define COPY_FILE

#include <sys/stat.h>

#include "hash_table.h"

// copies an element
void copy_elem(const char *dest, char *src, const char *orig_dir,
               const struct stat st, hash_table ht);

// copies a directory
void copy_dir(const char *dest_dir, char *src, const char *orig_dir, struct stat st);

#endif // COPY_FILE
