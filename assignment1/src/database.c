#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/linear_hashing.h"
#include "../include/list.h"
#include "../include/utilities.h"

// function that destroys a postalcode and returns the bytes freed
// needed to create the 1-d list
size_t destroy_postcode_node(void* p)
{
    const postcode post = (postcode)p;

    size_t bytes = 0;
    bytes += list_destroy(post->voters);
    bytes += sizeof(*post);
    free(post);

    return bytes;
}

// the default hash function of the paper on the K22 site
hash_t hash_int_default(int val) { return (hash_t)val; }

// function that expands the size of the ht by 1
size_t expand_one(size_t val) { return val+1; }

// function that expands the size of the ht by doubling it
size_t expand_double(size_t val) { return val*2; }


size_t get_participants_size(const database db)  { return hash_size(db->ht); }

size_t get_voters_size(const database db)  { return db->voters_num; }

void db_sort(const database db)
{
    // if not already sorted sort it
    if (db->sorted == false)
    {
        list_sort(db->list, comp_list);
        db->sorted = true;
    }
}

database db_create(const size_t bucket_size, const size_t st_capacity, const int expand_func)
{
    const database db = custom_malloc(sizeof(*db));

    // initialize data structures
    db->ht = hash_create(st_capacity, bucket_size, hash_int_default, (expand_func == 2)? expand_double: expand_one);
    db->list = list_create(destroy_postcode_node);

    db->voters_num = 0;
    return db;
}

bool db_insert(const database db, const voter v)
{
    // try to insert the voter into the database
    if (hash_insert(db->ht, v))
    {
        // also insert in the 2d list
        list_insert_postcode(db->list, v);
        v->voted = 'y';
        db->voters_num++;
        db->sorted = false;
        return true;
    }
    // voter already exists
    return false;
}

bool db_participant_insert(const database db, const voter v)
{
    return hash_insert(db->ht, v);
}

void db_insert_voter(const database db, const voter v)
{
    v->voted = 'y';
    list_insert_postcode(db->list, v);
    db->sorted = false;
    db->voters_num++;
}

bool db_mark_voted(const database db, const int pin)
{
    const voter v = hash_search(db->ht, pin);
    if (v != NULL)
    {
        if (v->voted == 'n')  // if voter exists and has not voted, mark him as voted
        {
            list_insert_postcode(db->list, v);
            db->sorted = false;
            v->voted = 'y';
            db->voters_num++;
        }
        return true;
    }
    return false;
}

size_t db_close(const database db)
{
    const size_t total_bytes = sizeof(*db) + list_destroy(db->list) + hash_destroy(db->ht);
    free(db);
    return total_bytes;
}
