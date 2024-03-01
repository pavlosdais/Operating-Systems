#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "common.h"
#include "compare_utils.h"
#include "hash_table.h"
#include "utilites.h"
#include "vector.h"
#include "compare_utils.h"

#define PERMS 777

// function prototype
char *make_dest(const char *dest, char *src, const char *orig_dir);

void copy_reg_file(const char *dest_dir, char *src, const char *orig_dir, struct stat st)
{
    char *dest = make_dest(dest_dir, src, orig_dir);

    // open source and destination directories
    const int fd_dst = open(dest, O_CREAT | O_EXCL | O_WRONLY, st.st_mode);
    CHECK(fd_dst, -1, "copy_reg_file-open");

    const int fd_src = open(src, O_RDONLY);
    CHECK(fd_src, -1, "copy_reg_file-open")

    char *buf = custom_malloc(BUF_SIZE*sizeof(char));
    do {
        const ssize_t bytes_read = read(fd_src, buf, BUF_SIZE);
        CHECK(bytes_read, -1, "copy_reg_file-read");

        if (bytes_read == 0) break;
        
        ssize_t wr = write(fd_dst, buf, bytes_read);
        CHECK(wr, -1, "copy_reg_file-write");
    }
    while (true);

    // free memory used and close the directories
    int r = close(fd_dst);
    CHECK(r, -1, "copy_reg_file-close");

    r = close(fd_src);
    CHECK(r, -1, "copy_reg_file-close");

    free(dest);
    free(buf);
}

void copy_dir(const char *dest_dir, char *src, const char *orig_dir, struct stat st)
{
    char *dest = make_dest(dest_dir, src, orig_dir);

    int r = mkdir(dest, st.st_mode);
    CHECK(r, -1, "copy_dir-mkdir");

    free(dest);
}


void copy_sym_link(const char *dest_dir, char *src, const char *orig_dir)
{
    // full path of original directory we copy symlink from
    char *orig_dir_full = get_path(orig_dir);

    // full path of destination directory
    char *full_dest_dir = get_path(dest_dir);

    // file pointed to by simlink symlink 
    char *target        = get_link_target(src);

    char *full_src      = get_link_full_path(src);
    char *target_fpath  = get_target_full_path(src, full_src);

    // destination symlink is placed at
    char *dest          = make_dest(dest_dir, src, orig_dir);

    // symlink target 
    char *target_dest   = make_dest(full_dest_dir, target_fpath, orig_dir_full);

    // failed to remove target dest symlink points outside the hierarchy
    // point to its full path
    if (target_dest == NULL) {
        const int r = symlink(target_fpath, dest);
        CHECK(r, -1, "copy_sym_link-symlink");
    }
    // points inside the hierarchy and is relative path 
    // use same relative path
    else if (!is_full_path(target)) {
        const int r = symlink(target, dest);
        CHECK(r, -1, "copy_sym_link-symlink");
    }
    // is full path use path with src directory changed to dest directory
    else {
        const int r = symlink(target_dest, dest);
        CHECK(r, -1, "copy_sym_link-symlink");
    }

    // destroy memory used
    free(target_fpath);
    free(dest);
    free(target);
    free(target_dest);
    free(full_dest_dir);
    free(orig_dir_full);
    free(full_src);
}

void copy_hard_link(const char *dest_dir, char *src, const char *orig_dir,
                    char *target)
{
    char *dest      = make_dest(dest_dir, src, orig_dir);
    char *dest_targ = make_dest(dest_dir, target, orig_dir);

    const int r = link(dest_targ, dest);
    CHECK(r, -1, "copy_hard_link-link");

    free(dest);
    free(dest_targ);
}

char *make_dest(const char *dest, char *src, const char *orig_dir)
{
    char *str = str_remove(src, orig_dir);    
    if (str == NULL)
        return NULL;

    char *new = custom_malloc((strlen(str) + strlen(dest) + 2)*sizeof(char));

    strcpy(new, dest);
    if (new[strlen(new)-1] != '/')
        strcat(new, "/");
    strcat(new, str);

    free(str);

    return new;
}

void copy_elem(const char *dest, char *src, const char *orig_dir,
               const struct stat st, hash_table ht)
{
    // element is a directory
    if (is_directory(st)) {
        copy_dir(dest, src, orig_dir, st);
        return;
    }

    // element is a hard link
    else if (is_hard_link(st)) {
        file_info *finfo = create_finfo(src, st.st_ino);

        // try to insert the hard link 
        char *target     =  hash_insert(ht, finfo);
        if (target != NULL) {
            destroy_finfo(finfo);
            copy_hard_link(dest, src, orig_dir, target);
            return;
        }
    }

    // element is a regular file
    if (is_reg_file(st)) {
        copy_reg_file(dest, src, orig_dir, st);
    }

    // element is a symbolic link
    else if (is_sym_link(st)) {
        copy_sym_link(dest, src, orig_dir);
    }
}
