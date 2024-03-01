#pragma once
#include "utilities.h"
#include "common.h"

// gets command line arguments for the writer
bool open_cla_writer(int argc, char* argv[], char** file_name, int* recid, int* update_value, int* time, char** shmid, log_type*);

// find & update the readers/writers blocking the writer
void find_blocked_writers(shm_buffer shm, const size_t place, const pid_t pid, size_t* empty_space,
FILE *logger, const log_type log_lvl);

// find & update the readers/writers being blocked by the writer
void find_blocking_writers(shm_buffer shm, const size_t index, const size_t place, FILE *logger, const log_type log_lvl);
