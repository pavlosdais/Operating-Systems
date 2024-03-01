#include <asm-generic/errno-base.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "compare_files.h"
#include "compare_utils.h"
#include "utilites.h"
#include "vector.h"
#include "copy_file.h"
#include "hash_table.h"
#include "merge_files.h"
#include "parsing.h"


int main(int argc, char **argv)
{
    char *dirA = NULL;
    char *dirB = NULL;
    char *dest = NULL;
    bool force = false;

    // parse command line arguments
    parse_args(argc, argv, &dirA, &dirB, &dest, &force);

    // compare the directories
    if (dest == NULL)
        cmp_folders(dirA, dirB);
    
    // merge the directories
    else
        merge_directories(dirA, dirB, dest, force);
    
    return 0;
}
