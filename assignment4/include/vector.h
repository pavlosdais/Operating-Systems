#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

typedef struct Vector_ *Vector;

// initializes vector
Vector vector_initialize(size_t init_cap);

// inserts element at the back of the vector
void vector_push_back(Vector vect, char *name);

// returns the element at specified index
char *vector_at(Vector vect, const size_t i);

// returns the size of the vector
size_t vector_size(Vector vect);

// destroys memory used by the vector
void vector_destroy(Vector vect);

#endif // VECTOR_H
