#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>

#include "common.h"

char *get_link_full_path(const char *path);

bool is_directory(const struct stat st)
{
    return (st.st_mode & S_IFMT) == S_IFDIR;
}

bool is_sym_link(const struct stat st)
{
    return (st.st_mode & S_IFMT) == S_IFLNK;
}

bool is_hard_link(const struct stat st)
{
    return st.st_nlink > 1;
}

bool is_reg_file(const struct stat st)
{
    return (st.st_mode & S_IFMT) == S_IFREG;
}

char *make_path(const char *path, char *name)
{
    size_t name_l = strlen(name) + 1;
    size_t path_l = strlen(path);

    char *buf = custom_malloc((name_l + path_l + 2)*sizeof(char));

    strcpy(buf, path);
    if(buf[strlen(buf)-1] != '/')
        strcat(buf, "/");
    strcat(buf, name);

    return buf;
}

char *str_remove(const char *str, const char *sub) 
{
    const size_t str_len = strlen(str);
    const size_t sub_len = strlen(sub);
    
    if (str_len <= sub_len)
        return NULL;

    for (size_t i = 0; i < sub_len; i++) {
        if(str[i] != sub[i]) {
            return NULL;
        }
    }

    for (size_t i = 0; i < str_len; i++) {
        if (str[i] != sub[i]) {
            char *new = custom_malloc((str_len - i) * sizeof(char));

            strcpy(new, str + i + 1);

            return new;
        }
    }

    assert(false);
    return NULL;
}

file_info *create_finfo(char *path, ino_t inode)
{
    file_info *finfo = custom_malloc(sizeof(file_info));

    finfo->path      = custom_malloc((strlen(path)+1)*sizeof(char));
    strcpy(finfo->path, path);
    finfo->inode_num = inode;

    return finfo;
}

void destroy_finfo(file_info *finfo)
{
    free(finfo->path);
    free(finfo);
}

void print_file_stats(struct stat file_stat, const char *file_name)
{
    printf("File Information for: %s\n", file_name);
    printf("File Size: %lld bytes\n",    (long long)file_stat.st_size);
    printf("Number of Blocks: %lld\n",   (long long)file_stat.st_blocks);
    printf("Last Access Time: %s",       ctime(&file_stat.st_atime));
    printf("Last Modification Time: %s", ctime(&file_stat.st_mtime));
    printf("File Permissions: %o\n",     file_stat.st_mode & 0777);
}

int compare_timespec(struct timespec t1, struct timespec t2)
{
    return (t1.tv_sec == t2.tv_sec)? t1.tv_nsec - t2.tv_nsec: t1.tv_sec - t2.tv_sec;
}

char* get_path(const char *path)
{
    char *buf = custom_malloc(PATH_MAX*sizeof(char));
    buf = realpath(path, buf);
    CHECK(buf, NULL, "get_path-realpath");

    return buf;
}

void remove_last_str(char *path)
{
    int len = strlen(path);
    for(int i = len-1; i >= 0; i--) {
        if(path[i] != '/')
            continue;

        path[i+1] = '\0';
        break;
    }
}

char *get_relative_path(const char *dir, const char *relative_path)
{
    assert(relative_path[0] != '/');

    char *temp = custom_malloc(PATH_MAX*sizeof(char));
    strcpy(temp, relative_path);

    char *path = custom_malloc(PATH_MAX*sizeof(char));
    strcpy(path, dir);

    if (path[strlen(path)-1] == '/') {
        path[strlen(path)-1] = '\0';
    }

    char *ptr = temp;
    char *tok;
    while ((tok = strtok(ptr, "/")) != NULL) {
        ptr = NULL;
        if (!strcmp(tok, ".")) {
            if(path[strlen(path)-1] != '/')
                strcat(path, "/");
            continue;
        }
        else if (!strcmp(tok, "..")) {
            remove_last_str(path);
        }
        else {
            strcat(path, tok);
            strcat(path, "/");
        }
    }

    // null terminate
    if (path[strlen(path)-1] == '/')
        path[strlen(path)-1] = '\0';
    else
        path[strlen(path)] = '\0';

    free(temp);

    return path;
}

bool is_full_path(const char *path)
{
    return path[0] == '/';
}

char* get_link_target(const char *path)
{
    struct stat stat_link;
    int r = lstat(path, &stat_link);
    CHECK(r, -1, "_get_link_target-stat");

    ssize_t buf_size = stat_link.st_size ? stat_link.st_size + 1 : PATH_MAX;

    char *buf = custom_calloc(buf_size, sizeof(char));

    ssize_t read = readlink(path, buf, buf_size);
    CHECK(read, -1, "_get_link_target-readlink");

    return buf;
}

char *get_target_full_path(const char *path, const char *src)
{
    char *fpath = get_link_target(path); 

    if(is_full_path(fpath))
        return fpath;

    // get directory src is in
    char *dir = custom_malloc(PATH_MAX*sizeof(char));
    strcpy(dir, src);
    remove_last_str(dir);

    // get relative path from that directory
    char *rel_path = get_relative_path(dir, fpath);
    char *full_path = get_link_full_path(rel_path);

    // destroy memory used and return the path
    free(fpath);
    free(rel_path);
    free(dir);
    return full_path;
}

char *get_link_full_path(const char *path)
{
    char *temp = custom_malloc(PATH_MAX*sizeof(char));
    strcpy(temp, path);

    if (is_full_path(path))
        return temp;

    remove_last_str(temp);

    char *fpath = get_path(temp);
    for (ssize_t i = strlen(path)-1; i >= 0; i--) {
        if(path[i] != '/')
            continue;

        strcat(fpath, &path[i]);

        break;
    }

    free(temp);
    return fpath;
}
