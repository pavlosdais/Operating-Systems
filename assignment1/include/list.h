#pragma once  // include at most once

#include "../include/types.h"
#include <stdbool.h>


// pointer to function that destroys an element value and returns the bytes freed by it
typedef size_t (*DestroyFunc)(void* value);

// compare function
typedef int (*CompareFunc)(void* a, void* b);

// list handle - abstraction
typedef struct listSet* List;

// General functions:

// creates a list
List list_create(const DestroyFunc);

// pushes value at the top of the list
void list_push(const List, void*);

// returns the size of the list
size_t list_size(const List);

// returns the value at the top of the list
void* list_top_value(const List);

// sorts the list using merge sort
void list_sort(const List, const CompareFunc);

// destroys the memory used by the list
// and return the number of bytes destroyed
size_t list_destroy(const List);


// Custom functions used for the commands:

// custom function that inserts a voter on our 2d list
void list_insert_postcode(const List list, const voter);

// print all participants with the specified zip
void print_zipcodes(const List, const int);

// print the list
void sorted_print(const List);
