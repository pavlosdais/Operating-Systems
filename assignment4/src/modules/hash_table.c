#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

// when to split
#define LAMDA_SPLIT 0.75

// wrap the data in a struct
typedef struct data_info
{
    data_t key;
}
data_info;

typedef struct _node* node;
struct _node
{
    data_info* data;       // the bucket we are storing
    uint32_t number_used;  // number of elements used in the bucket
    node next_bucket;      // overflow bucket
};

struct _linear_hash
{
    node* nodes;           // the array of nodes
    size_t p;              // the next bucket to be split
    size_t i;              // the exponenent of the hash function
    size_t elements_num;   // the number of elements stored currently in the hash table
    size_t curr_capacity;  // the current amount of buckects being used
    size_t max_capacity;   // the maximum number of buckects that can be used (allocated)
    size_t capacity;       // the maximum capacity of elements in non-overflow buckets
    size_t bucket_size;    // the number of elements that can fit in a bucket
    size_t powi;           // 2^i * m
    size_t powi_1;         // 2^(i+1) * m
    HashFunc hash;         // hash function
    ExpandFunc expand;     // expand function
};

// by default grow by one
size_t _my_default_expand(size_t num)  { return num + 1; }

hash_table hash_create(const size_t m, const size_t bucket_size, const HashFunc hash, const ExpandFunc expand)
{
    // the user failed to give a hash function
    if (hash == NULL) return NULL;

    hash_table ht = custom_calloc(1, sizeof(*ht));

    ht->expand = ((expand != NULL)? expand: _my_default_expand);

    ht->curr_capacity = m;

    ht->max_capacity = ht->expand(m);
    if (m > ht->max_capacity) ht->max_capacity = _my_default_expand(m);

    ht->nodes = custom_calloc(ht->max_capacity, sizeof(*ht->nodes));

    ht->bucket_size = bucket_size;
    ht->hash = hash;
    ht->powi = m;      // i = 0 so 2^0 * m = 1 * m = m
    ht->powi_1 = 2*m;  // i = 1 so 2^1 * m = 2 * m
    ht->capacity = ht->curr_capacity * ht->bucket_size;
    return ht;
}

static inline float calculate_lamda(const hash_table ht)
{
    // elemnts / capacity in non overflow buckets
    return (float)ht->elements_num / ht->capacity;
}

size_t hash_size(const hash_table ht)  { return ht->elements_num; }

// allocates memory for a bucket
static inline node create_bucket(const hash_table ht)
{
    const node new_bucket = custom_malloc(sizeof(*new_bucket));
    new_bucket->data = custom_malloc(ht->bucket_size * sizeof(*new_bucket->data));
    new_bucket->number_used = 0;
    new_bucket->next_bucket = NULL;
    return new_bucket;
}

// insert bucket at the specified index
static inline void insert_bucket(const hash_table ht, const size_t index)
{
    const node new_bucket = custom_malloc(sizeof(*new_bucket));
    new_bucket->data = custom_malloc(ht->bucket_size * sizeof(*new_bucket->data));
    new_bucket->number_used = 0;

    new_bucket->next_bucket = ht->nodes[index];
    ht->nodes[index] = new_bucket;
}

// calculate the hash value of the key
static inline hash_t calculate_hash(const hash_table ht, const value_t key)
{
    const hash_t hash = ht->hash(key);

    // use h_1
    const hash_t h_1 = hash % ht->powi;

    // use h_2 (if h_1 < p)
    return (h_1 < ht->p? hash % ht->powi_1: h_1);
}

// at split we only use h_2
static inline hash_t calculate_hash_split(const hash_table ht, const value_t key)
{
    return ht->hash(key) % ht->powi_1;
}

// search the ht for the specified key
static inline data_t hash_exists(const hash_table ht, const value_t key, const hash_t hash_value)
{
    node curr_bucket = ht->nodes[hash_value];
    while (curr_bucket != NULL)
    {
        for (size_t i = 0; i < curr_bucket->number_used; i++)
        {
            // compare the values
            if (get_key(curr_bucket->data[i].key) == key)  // key matches
                return curr_bucket->data[i].key;
        }
        curr_bucket = curr_bucket->next_bucket;
    }
    
    return NULL;
}

data_t hash_search(const hash_table ht, const value_t key)
{
    return hash_exists(ht, key, calculate_hash(ht, key));
}

// resize the hash table
// we expand by one the max capacity. we could double but setting as zero the bytes after the resize is expensive
static void hash_resize(const hash_table ht)
{
    ht->curr_capacity++;
    if (ht->curr_capacity == ht->max_capacity)
    {
        const size_t old_capacity = ht->curr_capacity;
        const size_t new_capacity = ht->expand(ht->curr_capacity);
        
        // check if the new size the user gave is valid
        // if not valid, use the default grow function
        ht->max_capacity = (old_capacity < new_capacity)? new_capacity: _my_default_expand(ht->curr_capacity);

        ht->nodes = realloc(ht->nodes, sizeof(struct _node) * ht->max_capacity);
        CHECK(ht->nodes, NULL, "hash_resize-realloc");

        memset(ht->nodes+old_capacity, 0, (ht->max_capacity - old_capacity) * sizeof(struct _node));
    }

    ht->capacity += ht->bucket_size;  // incrementally update the capacity in non overflow buckets
}

// insert bucket at the specified bucket
static inline void temp_insert(const hash_table ht, const data_t key, node* bucket)
{
    if (*bucket != NULL && (*bucket)->number_used < ht->bucket_size)  // element can be inserted at the bucket
        (*bucket)->data[(*bucket)->number_used++].key = key;
    else  // no empty spots found, create an overflow bucket
    {
        const node new_bucket = custom_malloc(sizeof(*new_bucket));
        new_bucket->data = custom_malloc(ht->bucket_size * sizeof(*new_bucket->data));
        new_bucket->data[0].key = key;
        new_bucket->number_used = 1;
        
        new_bucket->next_bucket = *bucket;
        *bucket = new_bucket;
    }
}

// split operation
static inline void bucket_split(const hash_table ht)
{
    // we split with the new index and p
    const size_t new_index = ht->curr_capacity-1;

    // buckets that will be splitted between p & new_index
    node buckets = ht->nodes[ht->p];

    node new_buckets = create_bucket(ht);
    insert_bucket(ht, new_index);

    while (buckets != NULL)
    {
        for (size_t i = 0; i < buckets->number_used; i++)
        {
            // hashing does not map back to the old bucket
            if (calculate_hash_split(ht, get_key(buckets->data[i].key)) != ht->p)
                temp_insert(ht, buckets->data[i].key, &ht->nodes[new_index]);
            else
                temp_insert(ht, buckets->data[i].key, &new_buckets);
        }
        
        const node tmp = buckets;
        buckets = buckets->next_bucket;
        free(tmp->data);
        free(tmp);
    }

    ht->nodes[ht->p] = new_buckets;
}

char *hash_insert(const hash_table ht, const data_t value)
{
    // find the bucket where the value should be inserted to
    const hash_t hash = calculate_hash(ht, get_key(value));

    // check if the value exists before inserting to avoid duplicates
    // in an implementation where it's guranteed that no duplicates exist 
    // we could comment out this line of code and always return true for extra speed
    data_t exists = hash_exists(ht, get_key(value), hash);
    if(exists != NULL) {
        return exists->path;
    }
    
    // insert value
    temp_insert(ht, value, &ht->nodes[hash]);

    // value inserted
    ht->elements_num++;
    
    // lamda exceeded the limit, split
    if (calculate_lamda(ht) > LAMDA_SPLIT)
    {
        // hash table needs resizing
        hash_resize(ht);

        // split the bucket
        bucket_split(ht);

        // start new round of splitting
        if (ht->powi_1 <= ht->curr_capacity)
        {
            ht->powi = ht->powi_1;  // 2^(i+1) = 2^i * 2
            ht->powi_1 *= 2;
            ht->p = 0;
        }
        else ht->p++;
    }

    // value inserted, increment the number of elements and return true
    return NULL;
}

void hash_print(const hash_table ht)
{
    for (size_t i = 0; i < ht->curr_capacity; i++)
    {
        printf("Bucket %ld | ", i);
        node curr_bucket = ht->nodes[i];
        size_t buckets_num = 0;
        while (curr_bucket != NULL)
        {
            if (buckets_num > 0) printf(" -Overflow bucket- ");

            for (size_t j = 0; j < curr_bucket->number_used; j++)
                printf("%ld ", get_key(curr_bucket->data[j].key));
            
            curr_bucket = curr_bucket->next_bucket;
            buckets_num=1;
        }

        printf("\n");
    }
}

void hash_destroy(const hash_table ht)
{
    for (size_t i = 0; i < ht->curr_capacity; i++)
    {
        // scan every overflow bucket
        node curr_bucket = ht->nodes[i];
        while (curr_bucket != NULL)
        {
            for (size_t j = 0; j < curr_bucket->number_used; j++)
            {
                free(curr_bucket->data[j].key->path);
                free(curr_bucket->data[j].key);
            }
            
            const node tmp = curr_bucket;
            curr_bucket = curr_bucket->next_bucket;

            free(tmp->data);
            free(tmp);
        }
    }
    free(ht->nodes);
    free(ht);
}
