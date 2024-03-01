#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>

// the size of the array containing the process ids
#define PID_ARR_SIZE (128)

// the logger
#define LOG_FILE "logging_info.log"

#define BLOCKSZ (4096)
#define BUF_SIZE BLOCKSZ
#define ERROR_CODE (-1)
#define MAX_UPDATE_AMMOUNT (50000)

#define EMPTY_PID (-1)
#define MICRO_T (1000000)

typedef long time_t;

struct _record
{
    int id;          // id of the client
    char lname[20];  // last name
    char fname[20];  // first name
    int balance;     // remaining balance
};

typedef struct _record* Record;

typedef struct create_info
{
    int numbers;  // number of readers/writers to be created
    char* time;   // maximum number of run time
    char* path;   // path to the executable
}
create_info;

typedef enum { READER = 0, WRITER, EMPTY, OCCUPIED } op_type;
typedef enum { SIMPLE_LOG = 1, MED_LOG, ADVANCED_LOG } log_type;

typedef struct process_info
{
    size_t start;       // the start range
    size_t end;         // the end range (empty for writer)
    sem_t sem;          // the semaphore given
    op_type type;       // reader/writer
    size_t ticket;      // the ticket the proccess holds
    size_t blocked_by;  // the number of readers/writers its being blocked by
    pid_t pid;          // the process id of the process
}
process_info;

// shared memory segment
// here we keep every information related to the shared segment
// eg. statistics and process info
struct _shmbuf
{
    // section 1: needed semaphores
    sem_t in;
    sem_t mx;
    sem_t wrt;
    sem_t max_entered;
    sem_t update_w;
    sem_t update_r;
    sem_t read_sem;

    // section 2: statistics
    size_t total_readers;     // 1. total number of readers
    time_t read_time;         // 2. total read time
    size_t total_writers;     // 3. total number of writers
    time_t write_time;        // 4. total write time
    time_t max_stall;         // 5. maximum number of stall time
    size_t num_processed;     // 6. total number of records processed

    // section 3: information about the current readers/writers
    size_t curr_size;
    process_info readers[PID_ARR_SIZE];  // information about the readers
    process_info writers[PID_ARR_SIZE];  // information about the writers
    size_t curr_readers;                 // current number of readers
    size_t curr_writers;                 // current number of writers

    size_t curr_ticket;
};
typedef struct _shmbuf* shm_buffer;

// permission declarations for shared memory & files
#define CODE_SHMOPEN (O_CREAT | O_EXCL | O_RDWR)
#define COPE_MMAP (PROT_READ | PROT_WRITE)
#define WRITE_PERM (O_RDWR)
#define READ_PERM (O_RDONLY)
#define APPEND_PERM (O_APPEND)
