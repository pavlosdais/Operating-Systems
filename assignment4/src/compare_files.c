#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include "common.h"
#include "compare_utils.h"
#include "utilites.h"
#include "vector.h"

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

static void descend_dir(const char *pathname, Vector vec)
{
    // open directory
    DIR *dir = opendir(pathname);
    CHECK(dir, NULL, "recurse-opendir");

    struct dirent **namelist;
    int num = scandir(pathname, &namelist, filter, cmp) - 1;

    // traverse simutaniously
    while (num >= 0) {
        // make paths to call stat
        char *path_buf = make_path(pathname, namelist[num]->d_name);

        struct stat st_A;
        int r = lstat(path_buf, &st_A);
        CHECK(r, -1, "descend_dir-stat");

        vector_push_back(vec, path_buf);

        // if both are directories descent simutaniously
        if (is_directory(st_A)) {
            descend_dir(path_buf, vec);
        }

        free(namelist[num--]);
    }

    // free memory used and close directory
    free(namelist);
    const int r = closedir(dir);
    CHECK(r, -1, "recurse-closedir");
}

static inline void diff_to_vec(const char *path, struct dirent *dr, int *idx, Vector vec)
{
    char *buf = make_path(path, dr->d_name);
    vector_push_back(vec, buf);

    struct stat st;
    int r = lstat(buf, &st);
    CHECK(r, -1, "diff_to_vec-stat");

    if (is_directory(st)) {
        descend_dir(buf, vec);
    }

    free(dr);
    (*idx)--;
}

static void 
recurse(const char *pathname_A, Vector vec_a, const char *dirA,
        const char *pathname_B, Vector vec_b, const char *dirB)
{
    DIR *dir_A = opendir(pathname_A);
    CHECK(dir_A, NULL, "recurse-opendir");

    DIR *dir_B = opendir(pathname_B);
    CHECK(dir_B, NULL, "recurse-opendir");

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
            diff_to_vec(pathname_A, namelist_A[num_A], &num_A, vec_a);
            continue;
        }
        // same here but with B
        else if (cmp > 0) {
            diff_to_vec(pathname_B, namelist_B[num_B], &num_B, vec_b);
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
        if(is_directory(st_A) && is_directory(st_B)) {
            recurse(path_buf_A, vec_a, dirA, path_buf_B, vec_b, dirB);
            free(path_buf_A);
            free(path_buf_B);
        }
        // compare the files
        else {
            if (!elem_eq(path_buf_A, st_A, dirA, path_buf_B, st_B, dirB)) {
                vector_push_back(vec_a, path_buf_A);
                if (is_directory(st_A))
                    descend_dir(path_buf_A, vec_a);
                
                vector_push_back(vec_b, path_buf_B);
                if (is_directory(st_B))
                    descend_dir(path_buf_B, vec_b);
            }
            else {
                free(path_buf_A);
                free(path_buf_B);
            }
        }

        free(namelist_A[num_A--]);
        free(namelist_B[num_B--]);
    }

    // now we need to check for the remaining entries
    // for both directories

    // case 1: remaining entries in A dont exist in B
    while (num_A >= 0) {
        diff_to_vec(pathname_A, namelist_A[num_A], &num_A, vec_a);
    }

    // case 2: remaining entries in B dont exist in A
    while (num_B >= 0) {
        diff_to_vec(pathname_B, namelist_B[num_B], &num_B, vec_b);
    }

    // destroy memory used and close directories
    free(namelist_A);
    free(namelist_B);

    int r = closedir(dir_A);
    CHECK(r, -1, "recurse-closedir");

    r = closedir(dir_B);
    CHECK(r, -1, "recurse-closedir");
}

void cmp_folders(const char *dirA, const char *dirB)
{
    // create the vectors that store 
    Vector vec_a = vector_initialize(VECTOR_SIZE); 
    Vector vec_b = vector_initialize(VECTOR_SIZE);

    // main compare function
    recurse(dirA, vec_a, dirA, dirB, vec_b, dirB);

    // print the differences
    const size_t size_a = vector_size(vec_a);
    const size_t size_b = vector_size(vec_b);

    if (size_a == 0 && size_b == 0) {
        printf("Directories %s and %s are the same\n", dirA, dirB);
        return;
    }
    
    // print differnces for directory A
    if (size_a > 0) {
        printf("In %s:\n", dirA);
        for (size_t i = 0; i < size_a; i++)
            printf("\t%s\n", vector_at(vec_a, i));
    }
        
    // print differnces for directory B
    if (size_b > 0) {
        printf("In %s:\n", dirB);
        for (size_t i = 0; i < size_b; i++)
            printf("\t%s\n", vector_at(vec_b, i));
    }

    // destroy memory used
    vector_destroy(vec_a);
    vector_destroy(vec_b);
}
