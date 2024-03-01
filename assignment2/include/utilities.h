#pragma once
#include <stdlib.h>
#include <stdio.h>
#include "common.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Safe routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// a safe execvp routine that exits in case of a failure
#define SAFE_EXECVP(argv, curr_file)                                                         \
    if (execvp(argv[0], arguments) == -1)                                                    \
    {                                                                                        \
        fprintf(stderr, "Error while trying to open the file at %s, exiting\n", curr_file);  \
        exit(EXIT_FAILURE);                                                                  \
    }                                                                                        \

// a safe fork routine that exits in case of a failure
#define SAFE_FORK(pid, curr_file)                                                   \
    if ((pid = fork()) == -1)                                                       \
    {                                                                               \
        fprintf(stderr, "Error while trying to fork at %s, exiting\n", curr_file);  \
        exit(EXIT_FAILURE);                                                         \
    }                                                                               \

// a safe poll routine that tries again in case of a failure
// that happens beacause sigaction blocks our poll
#define SAFE_POLL(fd, size, output, curr_file)  \
    if ((output = poll(fd, size, -1)) < 0)      \
        continue;                               \

// create signal action with the specified action function and signal
#define CREATE_SIGNAL_ACTION(signal_action, signal_action_func, sig_type)            \
    signal_action.sa_handler = signal_action_func;                                   \
    signal_action.sa_flags = SA_RESTART;                                             \
    sigfillset(&signal_action.sa_mask);                                              \
    if (sigaction(sig_type, &signal_action, NULL) == -1)                             \
    {                                                                                \
        fprintf(stderr, "Error in coordinator while trying to create sigaction\n");  \
        exit(EXIT_FAILURE);                                                          \
    }                                                                                \

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General use functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// create an array of pipes
int** create_pipes(const size_t);

// destroy the memory used by the pipes
void destroy_pipes(int**, const size_t);

// create a string that holds a number
char* int_to_string(int);

// returns a copy of the given string
char* alloc_n_cpy(char*, const size_t);

// creates a record array of the specified size
Record* create_record_array(size_t size);

// compare functions that compares two records and returns
// 1,  if a > b
// -1, if a > b
// 0,  if a = b
int compare_records(Record a, Record b);

// destroys a records array
void destroy_records(Record* record_arr, size_t size);

// creates an array indicating a range [start, end]
Range create_range(const size_t start, const size_t end);

// destroys the memory used for range
void destroy_range(const Range range);

// merges <array_num> number of arrays holding records into a new array
Record merge_records(Record* record_array, Range* ranges, const size_t array_num, size_t* final_size);

// a safe read routine repeatedly reading until all the bytes are read
void safe_read(void* source, const int pipe_num, size_t read_size);

// prints a record in the format given
void print_record(const Record record);

// creates a record array
Record* create_record_arr(const size_t, Range*);

// create poll arrays
struct pollfd* create_poll_arrays(int** pipes, const size_t number);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Coordinator functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// opens command line arguments for the coordinator
bool open_cla_coordinator(int argc, char* argv[], char**, size_t*, char**, char**);

// counts the number of records a file holds
size_t records_size(const char*);

// calculates the range will splitter will take on to sort
Range* calculate_splitter_range(const size_t file_size, const size_t num_of_children);

// prints the times of the sorters
void print_times(calculated_time**, const size_t);

// prints the final, merged records, of the file
// the difference between this function and `merge_records`
// is that this function does not create a merged array, only prints it
void print_merged(Record* record_array, Range* ranges, const size_t array_num);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Splitter functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// calculate the range the sorter will sort
Range* calculate_sorter_range(const size_t start, const size_t end, const size_t total_splitters);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sorter functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// swap nodes two given nodes
void swap_nodes(Record, Record);

// open the file with the specified name and a file pointer to it
// exits in case of failing to open the file
int open_file(const char*);
