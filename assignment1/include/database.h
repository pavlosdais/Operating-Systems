#pragma once

#include "types.h"

// creates the database
database db_create(const size_t, const size_t, const int);

// closes the database
size_t db_close(const database);

// insert voter into the database
bool db_insert(const database, const voter);

// mark the voter as voted
void db_insert_voter(const database, const voter);

// mark voter with the specified id as voted
bool db_mark_voted(const database, const int);

// get the number of participants
size_t get_participants_size(const database);

// get the number of voters
size_t get_voters_size(const database);

// sort the voters in the database
void db_sort(const database);

// insert participant in the db
bool db_participant_insert(const database, const voter);
