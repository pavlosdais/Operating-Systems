#define _XOPEN_SOURCE 700
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <ftw.h>

#include "common.h"
#include "compare_utils.h"
#include "utilites.h"
#include "vector.h"
#include "hash_table.h"
#include "copy_file.h"

// filters out current and previous path
static int filter(const struct dirent *dr)
{
    return strcmp(dr->d_name, ".") && strcmp(dr->d_name, "..");
}

// sorts files in alphabetical order
static int cmp(const struct dirent **dr1, const struct dirent **dr2)
{
    return strcmp((*dr2)->d_name, (*dr1)->d_name);
}

// descends a single directory, copying all of its elements
static void descend_dir(const char *pathname, const char *dest,
                        const char *orig_dir, hash_table ht)
{
    DIR *dir = opendir(pathname);
    CHECK(dir, NULL, "recurse-opendir");

    // get sorted
    struct dirent **namelist;
    int num = scandir(pathname, &namelist, filter, cmp) - 1;

    while (num >= 0) {
        // make paths to call stat
        char *path_buf = make_path(pathname, namelist[num]->d_name);

        struct stat st;

        int r = lstat(path_buf, &st);
        CHECK(r, -1, "descend_dir-stat");

        // copy element
        copy_elem(dest, path_buf, orig_dir, st, ht);

        // element was a directory, descend it
        if(is_directory(st)) {
            descend_dir(path_buf, dest, orig_dir, ht);
        }

        free(path_buf);
        free(namelist[num--]);
    }

    // destroy memory used and close the directory
    free(namelist);
    int r = closedir(dir);
    CHECK(r, -1, "recurse-closedir");
}

// copy the differences
static void copy_diff( const char *path, struct dirent *dr, int *idx,
                       const char *dest, const char *orig_dir, hash_table ht)
{
    char *buf;
    if (dr != NULL)
        buf = make_path(path, dr->d_name);
    else {
        buf = custom_malloc(strlen(path)+1);
        strcpy(buf, path);
    }

    struct stat st;
    int r = lstat(buf, &st);
    CHECK(r, -1, "copy_diff-stat");

    copy_elem(dest, buf, orig_dir, st, ht);

    if (is_directory(st)) {
        descend_dir(buf, dest, orig_dir, ht);
    }

    free(buf);
    free(dr);

    if(idx != NULL) (*idx)--;
}

// main merge function
static void recurse( const char *pathname_A, const char *dirA,
                     const char *pathname_B, const char *dirB,
                     const char *dest, hash_table ht)
{
    // open directory A
    DIR *dir_A = opendir(pathname_A);
    CHECK(dir_A, NULL, "recurse-opendir");

    // open directory B
    DIR *dir_B = opendir(pathname_B);
    CHECK(dir_B, NULL, "recurse-opendir");

    // get the files of both directories in a sorted manner
    struct dirent **namelist_A, **namelist_B;
    int num_A = scandir(pathname_A, &namelist_A, filter, cmp) - 1;
    int num_B = scandir(pathname_B, &namelist_B, filter, cmp) - 1;

    // traverse simutaniously
    while (num_A >= 0 && num_B >= 0) {
        const int cmp = strcmp(namelist_A[num_A]->d_name, namelist_B[num_B]->d_name);

        // advance A since it doesnt exist in B 
        // we know it doesnt exist because the list is sorted
        // and comparison didnt return 0
        if (cmp < 0) {
            copy_diff(pathname_A, namelist_A[num_A], &num_A, dest, dirA, ht);
            continue;
        }
        // same here but with B
        else if (cmp > 0) {
            copy_diff(pathname_B, namelist_B[num_B], &num_B, dest, dirB, ht);
            continue;
        }

        // make paths to call stat
        char *path_buf_A = make_path(pathname_A, namelist_A[num_A]->d_name);
        char *path_buf_B = make_path(pathname_B, namelist_B[num_B]->d_name);

        struct stat st_A, st_B;

        // Use lstat in case we have links
        int r = lstat(path_buf_A, &st_A);
        CHECK(r, -1, "recurse-stat");

        lstat(path_buf_B, &st_B);
        CHECK(r, -1, "recurse-stat");

        // if both are directories, descent simutaniously
        if (is_directory(st_A) && is_directory(st_B)) {
            copy_dir(dest, path_buf_A, dirA, st_A);
            recurse(path_buf_A, dirA, path_buf_B, dirB, dest, ht);
        }
        // compare the files
        else {
            if (!elem_eq(path_buf_A, st_A, dirA, path_buf_B, st_B, dirB)) {

                // elements have the same name, copy the most recently modified one
                const int cmp = compare_timespec(st_A.st_mtim, st_B.st_mtim);

                if (cmp >= 0) copy_diff(path_buf_A, NULL, NULL, dest, dirA, ht);
                else          copy_diff(path_buf_B, NULL, NULL, dest, dirB, ht);
            }
            else
                copy_elem(dest, path_buf_A, dirA, st_A, ht);
        }

        free(path_buf_A);
        free(path_buf_B);
        free(namelist_A[num_A--]);
        free(namelist_B[num_B--]);
    }

    // now we need to check for the remaining entries
    // for both directories

    // case 1: remaining entries in A dont exist in B
    while (num_A >= 0) {
        copy_diff(pathname_A, namelist_A[num_A], &num_A, dest, dirA, ht);
    }

    // case 2: remaining entries in B dont exist in A
    while (num_B >= 0) {
        copy_diff(pathname_B, namelist_B[num_B], &num_B, dest, dirB, ht);
    }

    // destroy memory used and close directories
    free(namelist_A);
    free(namelist_B);

    int r = closedir(dir_A);
    CHECK(r, -1, "recurse-closedir");

    r = closedir(dir_B);
    CHECK(r, -1, "recurse-closedir");
}

// a simple hash function
hash_t hash(int x)  { return x; }

// function needed by the function "nftw" at remove_dir
int remove_fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    (void) typeflag;
    (void) ftwbuf;
    
    int r = -1;

    if (is_directory(*sb)) {
        r = rmdir(fpath);
        CHECK(r, -1, "remove_fn-rmdir");
    }
    else {
        r = unlink(fpath);
        CHECK(r, -1, "remove_fn-unlink");
    }

    return r;
}

void remove_dir(const char *path)
{
    const int r = nftw(path, remove_fn, 16, FTW_DEPTH | FTW_PHYS);
    CHECK(r, -1, "remove_dir-nftw");
}

static inline void directory_exists(const char *dir)
{
    // try to open directory
    DIR *tmp_dir = opendir(dir);
    if (tmp_dir == NULL)
    {
        fprintf(stderr, "Error! Directory %s does not exist\n", dir);
        exit(EXIT_FAILURE);
    }
    
    // close directory
    const int r = closedir(tmp_dir);
    CHECK(r, -1, "directory_exists-closedir");
}

void merge_directories(const char *dirA, const char *dirB, const char *dest, const bool force)
{
    // check if the directories already exist
    directory_exists(dirA);
    directory_exists(dirB);

    // try to create the directory
    int r = mkdir(dest, S_IRWXU);
    if (r == -1) {
        bool flag = false;
        if (errno == EEXIST && !force) {
            fprintf(stderr, "Destination directory %s already exists\n", dest);
            exit(EXIT_FAILURE);
        }
        // directory already exists and user gave the option to delete it
        else if (errno == EEXIST && force) {
            remove_dir(dest);
            int rr = mkdir(dest, S_IRWXU);
            CHECK(rr, -1, "merge_directories-mkdir");
            flag = true;
        }

        if(!flag) {
            perror("merge_directories-mkdir"); 
            exit(EXIT_FAILURE);
        }
    }

    // create a hash table that stores the hard links
    hash_table ht = hash_create(HASH_SIZE, BUCKET_SIZE, hash, NULL);

    // merge operation
    recurse(dirA, dirA, dirB, dirB, dest, ht);

    printf("[Sucess] Merge successfully completed\n");

    // destroy the memory used by the hash table
    hash_destroy(ht);
}
