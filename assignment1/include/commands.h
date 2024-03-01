#pragma once

#include "types.h"

// 1 - l
void find_participant(const database);

// 2 - i
void insert_participant(const database db);

// 3 - m
void mark_voted(const database);

// 4 - bv
void voters_file(const database);

// 5 - v
void participants_num(const database db);

// 6 - perc
void vote_percentage(const database db);

// 7 - z
void zipcode_voters(const database db);

// 8 - o
void postcode_voters(const database db);

// 10 - p
void print_db(const database);
