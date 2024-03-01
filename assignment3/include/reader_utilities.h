#pragma once
#include "utilities.h"
#include "common.h"

// gets command line arguments for the reader
bool open_cla_reader(int argc, char* argv[], char** file_name, int* recid_st, int* recid_end, int* time, char** shmid, log_type*);

// find & update the writers blocking the reader
void find_blocked_readers(shm_buffer shm, const size_t start, const size_t end, const pid_t pid,
size_t* empty_space, FILE *logger, const log_type log_lvl);

// find & update the writers being blocked by the reader
void find_blocking_readers(shm_buffer shm, const size_t index, const size_t start, const size_t end,
FILE *logger, const log_type log_lvl);
