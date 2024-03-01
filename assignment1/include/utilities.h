#pragma once

#include "types.h"
#include "linear_hashing.h"
#include <stdbool.h>

// gets commands and returns the assigned mapped value
char command_num(char*);

// prints message to stderr 
void unsuccessful_response(const char*);

// prints message to stdout
void successful_response(const char*);

// get rid of white spaces
void command_preprocess(char*);

// gets as input a string and returns an equivalent integer
// returns -1 if not an integer
int string_to_int(const char*);

// creates voter
// input: <first name>  <last name> <ID> <zipcode>
voter create_voter(char*, char*, const int, const int);

// returns a copy of the given string
char* mystrcpy(const char*);

// opens command line arguments and creates the database
// returns NULL if an error occured while parsing the arguments
database open_cmd(int argc, char* argv[]);

// a compare function needed for the list sort
int comp_list(void*, void*);

// pint malformed input error and return false if the string is NULL
bool check_malformed(const char*);

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
