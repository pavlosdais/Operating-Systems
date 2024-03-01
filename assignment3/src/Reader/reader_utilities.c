#include <stdbool.h>
#include <string.h>
#include "../../include/common.h"
#include "../../include/utilities.h"
#include "../../include/reader_utilities.h"

// -f <filename> -l <recid[,recid]> -d <time> -s <shmid>
bool open_cla_reader(int argc, char* argv[], char** file_name, int* recid_st, int* recid_end, int* time, char** shmid, log_type* log_level)
{
    if (argc != 12) return false;

    *file_name = NULL; *recid_st = 0; *recid_end = 0;
    *time = 0;
    *shmid = NULL;
    
    for (int i = 1; i < argc-1; i+=2)
    {
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == 'f')  // -f <filename>
                *file_name = argv[i+1];
            else if (strcmp(argv[i], "-l") == 0)  // -l <recid>
            {
                *recid_st = atoi(argv[i+1]);
                *recid_end = atoi(argv[i+2]);
                i++;
            }
            else if (argv[i][1] == 'd')  // -d <time>
                *time = atoi(argv[i+1]);
            else if (argv[i][1] == 's')  // -s <shmid>
                *shmid = argv[i+1];
            else if (strcmp(argv[i], "-lg") == 0)  // -s <shmid>
                *log_level = atoi(argv[i+1]);
            else return false;
        }
        else return false;
    }
    return true;
}

void find_blocked_readers(shm_buffer shm, const size_t start, const size_t end, const pid_t pid, size_t* empty_space, FILE *logger, log_type log_lvl)
{
    assert(shm->curr_size < PID_ARR_SIZE);

    size_t blocked_by = 0, writers_rem = 0;
    *empty_space = PID_ARR_SIZE;

    sem_wait(&(shm->mx));
    
    for (size_t i = 0; i < PID_ARR_SIZE; i++)
    {
        // being blocked by writer
        if (shm->writers[i].type == OCCUPIED)
        {
            if (shm->writers[i].start >= start && shm->writers[i].start <= end)
            {
                blocked_by++;
                if (log_lvl > MED_LOG)
                {
                    fprintf(logger, "[%d]Reader (%ld,%ld) is being blocked by [%d]Writer\n", pid, start+1, end+1, shm->writers[i].pid);
                    fflush(logger);
                }
            }
            
            writers_rem++;
        }
        
        if (shm->readers[i].type == EMPTY && *empty_space == PID_ARR_SIZE)  // found an empty area
            *empty_space = i;
        
        // searched through all writers & found an empty spot, exit
        if (writers_rem == shm->curr_writers && *empty_space != PID_ARR_SIZE) break;
    }
    assert(*empty_space != PID_ARR_SIZE);

    // insert reader at empty spot
    shm->readers[*empty_space].type       = OCCUPIED;
    shm->readers[*empty_space].start      = start;
    shm->readers[*empty_space].end        = end;
    shm->readers[*empty_space].blocked_by = blocked_by;
    shm->readers[*empty_space].pid        = pid;
    shm->readers[*empty_space].ticket     = shm->curr_ticket++;
    
    shm->curr_readers++;
    shm->curr_size++;

    if (log_lvl > SIMPLE_LOG)
    {
        fprintf(logger, "[%d]Reader (%ld,%ld) entered at place %ld and is blocked by %ld writers\n",
                pid, start+1, end+1, *empty_space, blocked_by);
        fflush(logger);
    }

    sem_post(&(shm->mx));
    if (blocked_by != 0)
        sem_wait(&(shm->readers[*empty_space].sem));

    sem_wait(&(shm->mx));

    if (log_lvl > SIMPLE_LOG)  // using medium/advanced level
    {
        fprintf(logger, "[%d]Reader (%ld,%ld) passed the semaphore\n", pid, start+1, end+1);
        fflush(logger);
    }
    else
    {
        fprintf(logger, "[%d]Reader (%ld,%ld) entered\n", pid, start+1, end+1);
        fflush(logger);
    }

    // using advanced level
    if (log_lvl > MED_LOG)
    {
        print_active_readers(shm, logger);
        print_active_writers(shm, logger);
    }

    sem_post(&(shm->mx));
}

void find_blocking_readers(shm_buffer shm, const size_t index, const size_t start, const size_t end, FILE *logger, log_type log_lvl)
{
    size_t unblocked = 0, writers_rem = 0;

    sem_wait(&(shm->mx));

    shm->readers[index].type = EMPTY;

    const size_t my_ticket = shm->readers[index].ticket;
    
    for (size_t i = 0; i < PID_ARR_SIZE && writers_rem != shm->curr_writers; i++)
    {
        // blocking writer
        if (shm->writers[i].type == OCCUPIED)
        {
            if (shm->writers[i].ticket > my_ticket && (shm->writers[i].start >= start && shm->writers[i].start <= end))
            {
                assert(shm->writers[i].blocked_by > 0);
                
                if (log_lvl > MED_LOG)
                {
                    fprintf(logger, "[%d]Reader (%ld, %ld) was blocking [%d]Writer\n", shm->readers[index].pid, shm->readers[index].start+1, shm->readers[index].end+1, shm->writers[i].pid);
                    fflush(logger);
                }
                unblocked++;

                // writer is not being blocked by any process, wake him up
                if ((--(shm->writers[i].blocked_by)) == 0)
                    sem_post(&(shm->writers[i].sem));
            }
            writers_rem++;
        }
    }

    if (log_lvl > SIMPLE_LOG)
    {
        fprintf(logger, "[%d]Reader (%ld, %ld) exited after unblocking %ld writer(s)\n", shm->readers[index].pid, start, end, unblocked);
        fflush(logger);
    }
    else
    {
        fprintf(logger, "[%d]Reader (%ld, %ld) exited\n", shm->readers[index].pid, start+1, end+1);
        fflush(logger);
    }

    shm->curr_readers--;
    shm->curr_size--;

    if (log_lvl > MED_LOG)
    {
        print_active_readers(shm, logger);
        print_active_writers(shm, logger);
    }

    sem_post(&(shm->mx));
}
