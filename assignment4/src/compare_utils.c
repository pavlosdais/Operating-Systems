#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "common.h"
#include "utilites.h"
#include "compare_utils.h"

bool files_is_eq(const char *path_A, const struct stat stat_A,
                 const char *path_B, const struct stat stat_B)
{
    if(stat_A.st_size != stat_B.st_size)
        return false;

    int fd_A = open(path_A, O_RDONLY);
    CHECK(fd_A, -1, "files_is_eq-open");
    
    int fd_B = open(path_B, O_RDONLY);
    CHECK(fd_B, -1, "files_is_eq-open");

    ssize_t bytes_A, bytes_B;

    // create 2 buffers that will temporarily store the bytes
    char *buf_A = custom_malloc(BUF_SIZE*sizeof(char));
    char *buf_B = custom_malloc(BUF_SIZE*sizeof(char));

    bool r_value = true;
    do {
        // read contents of array A
        bytes_A = read(fd_A, buf_A, BUF_SIZE);
        CHECK(bytes_A, -1, "files_is_eq-read");

        // read contents of array B
        bytes_B = read(fd_B, buf_B, BUF_SIZE);
        CHECK(bytes_B, -1, "files_is_eq-read");

        // files differ becasue either
        // a) their size differs
        // b) their content is different
        if(bytes_A != bytes_B || memcmp(buf_A, buf_B, bytes_A) != 0) {
            r_value = false;
            break;
        }
    }
    while (bytes_A > 0);

    // destroy memory used and close directories
    free(buf_A);
    free(buf_B);

    int r = close(fd_A);
    CHECK(r, -1, "files_is_eq-close");

    r = close(fd_B);
    CHECK(r, -1, "files_is_eq-close");

    return r_value;
}

// target    is the links targets
// targ_path is the links targets with initial directory removal
// dir_A     is the initial directory
bool link_targ_is_eq(const char *target_A, const char *targ_path_A, const char *dir_A,
                     const char *target_B, const char *targ_path_B, const char *dir_B)
{
    // compare names
    if (strcmp(targ_path_A, targ_path_B)) return false;

    // check for dangling links
    bool is_dag_A = false;
    bool is_dag_B = false;

    struct stat st_A, st_B;
    int r = lstat(target_A, &st_A); 
    if(r == -1) {
        if(errno == ENOENT) {
            is_dag_A = true;        
        }
        else {
            perror("link_targ_is_eq-lstat");
            exit(EXIT_FAILURE);
        }
    }

    r = lstat(target_B, &st_B);
    if (r == -1) {
        if (errno == ENOENT)
            is_dag_B = true;
        else {
            perror("link_targ_is_eq-lstat");
            exit(EXIT_FAILURE);
        }
    }

    if (!is_dag_A && !is_dag_B)
        return elem_eq(target_A, st_A, dir_A, target_B, st_B, dir_B);
    else if (!is_dag_A || !is_dag_B)
        return false;
    else
        return !strcmp(targ_path_A, targ_path_B);
}

bool link_is_eq(const char *path_A, const char *path_B,
                const char *dir_A,  const char *dir_B)
{
    char *fpath_A  = get_link_full_path(path_A);
    char *fpath_B  = get_link_full_path(path_B);
    char *target_A = get_target_full_path(path_A, fpath_A);
    char *target_B = get_target_full_path(path_B, fpath_B);

    char *dirA     = get_path(dir_A);
    char *dirB     = get_path(dir_B);

    // Remove starting directory if we cant remove it 
    // then the link points outside the directory so 
    // compare original link targets
    char *targ_path_A = str_remove(target_A, dirA);
    char *targ_path_B = str_remove(target_B, dirB);

    if (targ_path_A == NULL && targ_path_B == NULL) {
        const int cmp = link_targ_is_eq(target_A, target_A, dirA, target_B, target_B, dirB);
        free(fpath_A);
        free(fpath_B);
        free(target_A);
        free(target_B);
        free(dirA);
        free(dirB);
        return cmp;
    }
    else if(targ_path_A == NULL || targ_path_B == NULL) {
        free(fpath_A);
        free(fpath_B);
        free(target_A);
        free(target_B);
        free(dirA);
        free(dirB);
        return false;
    }

    // destroy memory used & return if the links are equal
    const bool r_value = link_targ_is_eq(target_A, targ_path_A, dirA, target_B, targ_path_B, dirB);
    free(fpath_A);
    free(fpath_B);
    free(target_A);
    free(target_B);
    free(dirA);
    free(dirB);
    free(targ_path_A);
    free(targ_path_B);

    return r_value;
}

bool elem_eq(const char *pathname_A, struct stat stat_A, const char *dir_A,
             const char *pathname_B, struct stat stat_B, const char *dir_B)
{
    // both are directories
    if (is_directory(stat_A) && is_directory(stat_B)){
        return true;
    }

    // both are symbolic links
    if (is_sym_link(stat_A) && is_sym_link(stat_B)) {
        return link_is_eq(pathname_A, pathname_B, dir_A, dir_B);
    }

    // both are regular files
    if (is_reg_file(stat_A) && is_reg_file(stat_B)) {
        return files_is_eq(pathname_A, stat_A, pathname_B, stat_B);
    }

    return false;
}
