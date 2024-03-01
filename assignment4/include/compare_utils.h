#ifndef COMPARE_UTILS
#define COMPARE_UTILS

#include <stdbool.h>
#include <sys/stat.h>

// returns true if both elements (such as folders, regular files etc) are equal, false otherwise
bool 
elem_eq(const char *pathname_A, struct stat stat_A, const char *dir_A,
        const char *pathname_B, struct stat stat_B, const char *dir_B);

#endif // COMPARE_UTILS
