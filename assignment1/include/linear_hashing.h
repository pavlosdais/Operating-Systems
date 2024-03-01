#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

// value types we need
typedef unsigned int hash_t;
typedef int value_t;
typedef struct _voter* data_t;

// function that hashes a value
typedef hash_t (*HashFunc)(int value);

// function that takes as input the old size of buckets and outputs the new bucket size
typedef size_t (*ExpandFunc)(size_t value);


// hash table handle - abastraction
typedef struct _linear_hash* hash_table;

// how to get the key
#define get_key(val) (val->PIN)

// create hash table
// input: <initial number of buckets>, <bucket size>, <hash function>, <expand function>
// a hash function is necessary but the expand function can be NULL if the user wants to use
// the default expand function
hash_table hash_create(const size_t, const size_t, const HashFunc, const ExpandFunc);

// get the number of elements currently inserted in the hash table
size_t hash_size(const hash_table);

// insert value at the hash table
// returns true if the operation was successful, false if not
bool hash_insert(const hash_table, const data_t);

// search the hash table and return the element with the key
// NULL if not found
data_t hash_search(const hash_table, const value_t);

// print hash table (for debugging purposes)
void hash_print(const hash_table);

// destroy memory used by the hash table
// and return the number of bytes destroyed
size_t hash_destroy(const hash_table);
