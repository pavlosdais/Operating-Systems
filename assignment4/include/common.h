#ifndef COMMON_H
#define COMMON_H 

#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1024
#define VECTOR_SIZE 16
#define HASH_SIZE 5
#define BUCKET_SIZE 5

#define CHECK(v, errorv, name) \
    if(v == errorv) {          \
        perror(name);          \
        exit(EXIT_FAILURE);    \
    }                          \

// define a file by its path & its i-node number
typedef struct file_info
{
    char* path;
    ino_t inode_num;
}
file_info;

// wraps the standard malloc and checks if the allocation failed
static inline void* custom_malloc(const size_t alloc_size)
{
    void* res = malloc(alloc_size);
    if (res == NULL)
    {
        fprintf(stderr, "Memory allocation failed. Exiting..\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

// wraps the standard calloc and checks if the allocation failed
static inline void* custom_calloc(const size_t num_elements, const size_t size_element)
{
    void* res = calloc(num_elements, size_element);
    if (res == NULL)
    {
        fprintf(stderr, "Memory allocation failed. Exiting..\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

#endif // COMMON_H
