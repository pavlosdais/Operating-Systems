#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "vector.h"

#define CAP_GROWTH 2

typedef struct Vector_ {
    size_t capacity;
    size_t size;
    char **arr;
} Vector_;

Vector vector_initialize(size_t init_cap)
{
    Vector vect = custom_malloc(sizeof(Vector_));

    vect->size     = 0;
    vect->capacity = init_cap;
    
    vect->arr = custom_malloc(vect->capacity*sizeof(char*));

    return vect;
}

static void vector_resize(Vector vect)
{
    vect->capacity *= CAP_GROWTH;

    vect->arr = realloc(vect->arr, vect->capacity*sizeof(char*));
    CHECK(vect->arr, NULL, "vector_resize-realloc"); 
}

void vector_push_back(Vector vect, char *name)
{
    assert(name != NULL);
    
    if(vect->capacity == vect->size) {
        vector_resize(vect);
    }

    vect->arr[vect->size++] = name;
}

char *vector_at(Vector vect, const size_t i)
{
    assert(vect->size >= i);

    return vect->arr[i];
}

size_t vector_size(Vector vect)
{
    return vect->size;
}

void vector_destroy(Vector vect)
{
    for(size_t i = 0; i < vect->size; i++)
        free(vect->arr[i]);
    
    free(vect->arr);
    free(vect);
}
