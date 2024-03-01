#ifndef MERGE_FILES
#define MERGE_FILES

#include <stdbool.h>

// 
void merge_directories(const char *dirA, const char *dirB, const char *dest, const bool force);

// 
void remove_dir(const char *path);

#endif // MERGE_FILES
