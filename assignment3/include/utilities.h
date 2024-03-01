#pragma once
#include <sys/shm.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "common.h"

// a safe execvp routine that exits in case of a failure
#define SAFE_EXECVP(argv, curr_file, shm, shared_mem)                                        \
    if (execvp(argv[0], arguments) == -1)                                                    \
    {                                                                                        \
        fprintf(stderr, "Error while trying to open the file at %s, exiting\n", curr_file);  \
        shm_destroy(shm, shared_mem);                                                        \
        exit(EXIT_FAILURE);                                                                  \
    }                                                                                        \

// a safe fork routine that exits in case of a failure
#define SAFE_FORK(pid, curr_file, shm, shared_mem)                                  \
    if ((pid = fork()) == -1)                                                       \
    {                                                                               \
        fprintf(stderr, "Error while trying to fork at %s, exiting\n", curr_file);  \
        shm_destroy(shm, shared_mem);                                               \
        exit(EXIT_FAILURE);                                                         \
    }                                                                               \

#define SAFE_READ(fd, records, curr_file)                                   \
    const ssize_t bytes_read = read(fd, &record, sizeof(struct _record));   \
    if (bytes_read < 0)                                                     \
    {                                                                       \
        fprintf(stderr, "Error! Can not read records at %s\n", curr_file);  \
        exit(EXIT_FAILURE);                                                 \
    }                                                                       \

#define SAFE_WRITE(fd, record, curr_file)                                   \
    if (write(fd, &record, sizeof(struct _record)) == ERROR_CODE)           \
    {                                                                       \
        fprintf(stderr, "Error! Can not write record at %s\n", curr_file);  \
        exit(EXIT_FAILURE);                                                 \
    }                                                                       \

#define SAFE_SHARED_MEMORY(shm, shared_mem, curr_file)                          \
    shm = open_shm(shared_mem);                                                 \
    if (shm == NULL)                                                            \
    {                                                                           \
        printf("Error while trying to open shared memory at %s\n", curr_file);  \
        exit(EXIT_FAILURE);                                                     \
    }                                                                           \

#define CALC_TIME(start, end, result)                                 \
    const long _seconds_t = end.tv_sec - start.tv_sec;                \
    result = ((_seconds_t * 1000000) + end.tv_usec) - start.tv_usec;  \

#define GET_TIME(timer)          \
    gettimeofday(&timer, NULL);  \

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// gets command line arguments for the creator (creator.c)
bool open_cla_creator(int argc, char* argv[], char** file_name, char** shared_mem, create_info* readers, create_info* writers, char** log_level);

// converts integer to string
char* int_to_string(int);

// creates shared memory segment
shm_buffer shm_create(const char*);

// opens shared memory segment
shm_buffer open_shm(const char*);

// unlinks shared memory
void shm_destroy(shm_buffer, const char*);

// initializes POSIX semaphore
bool init_semaphore(sem_t*, const int);

// gets a random number for both start and end (start <= end)
void get_random_range(int* start, int* end, const int total_num);

// open files and returns its file descriptor
int open_file(const char*, const int);

// returns a random record
int get_random_record(const int);

// find the number of records in the file
size_t records_size(const char*);

// gets a random value [0, num-1]
int get_random_value(const int num);

// a safe routine that repeatedly reads the bytes until all are read
void safe_read(void*, const int fd, size_t read_size);

// reads a record (prints it)
void read_record(const pid_t process_id, const struct _record record);

// shows the statistics from reader/writer
void show_statistics(shm_buffer shm);

// prints the active readers (currently in the CS)
void print_active_readers(shm_buffer shm, FILE *logger);

// prints the active writers (currently in the CS)
void print_active_writers(shm_buffer shm, FILE *logger);
