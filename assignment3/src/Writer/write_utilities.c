#include <stdbool.h>
#include <string.h>
#include "../../include/common.h"
#include "../../include/utilities.h"
#include "../../include/write_utilities.h"

bool open_cla_writer(int argc, char* argv[], char** file_name, int* recid, int* update_value, int* time, char** shmid, log_type* log_level)
{
    // make sure the correct number of command line arguments is given
    if (argc != 13) return false;

    // look for the correct commandline arguments
    *file_name = NULL;
    *recid = 0; *update_value = 0;
    *time = 0;  *shmid = NULL;
    *log_level = 1;

    for (int i = 1; i < argc-1; i+=2)
    {
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == 'f')  // -f <filename>
                *file_name = argv[i+1];
            else if (strcmp(argv[i], "-l") == 0)  // -l <recid>
                *recid = atoi(argv[i+1]);
            else if (argv[i][1] == 'd')  // -d <time>
                *time = atoi(argv[i+1]);
            else if (argv[i][1] == 'v')  // -v <value>
                *update_value = atoi(argv[i+1]);
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

void find_blocked_writers(shm_buffer shm, const size_t place, const pid_t pid, size_t* empty_space, FILE *logger, const log_type log_lvl)
{
    assert(shm->curr_size < PID_ARR_SIZE);

    size_t blocked_by_r = 0, blocked_by_w = 0, readers_rem = 0, writers_rem = 0;
    *empty_space = PID_ARR_SIZE;

    sem_wait(&(shm->mx));
    
    for (size_t i = 0; i < PID_ARR_SIZE; i++)
    {
        // being blocked by reader
        if (shm->readers[i].type == OCCUPIED)
        {
            if (place >= shm->readers[i].start && place <= shm->readers[i].end)
            {
                blocked_by_r++;
                if (log_lvl > MED_LOG)
                {
                    fprintf(logger, "[%d]Writer (%ld) is being blocked by [%d]Reader\n", pid, place+1, shm->readers[i].pid);
                    fflush(logger);
                }
            }

            readers_rem++;
        }
        
        // being blocked by writer
        if (shm->writers[i].type == OCCUPIED)
        {
            if (shm->writers[i].start == place)
            {
                blocked_by_w++;
                if (log_lvl > MED_LOG)
                {
                    fprintf(logger, "[%d]Writer (%ld) is being blocked by [%d]Writer\n", pid, place+1, shm->writers[i].pid);
                    fflush(logger);
                }
            }
            
            writers_rem++;
        }
        else if (*empty_space == PID_ARR_SIZE)  // found an empty spot
            *empty_space = i;
        
        if (readers_rem == shm->curr_readers && writers_rem == shm->curr_writers && *empty_space != PID_ARR_SIZE) break;
    }
    assert(*empty_space != PID_ARR_SIZE);
    
    // insert writer at empty spot
    shm->writers[*empty_space].type       = OCCUPIED;
    shm->writers[*empty_space].start      = place;
    shm->writers[*empty_space].blocked_by = blocked_by_r+blocked_by_w;
    shm->writers[*empty_space].pid        = pid;
    shm->writers[*empty_space].ticket     = shm->curr_ticket++;
    
    shm->curr_writers++;
    shm->curr_size++;

    if (log_lvl > SIMPLE_LOG)
    {
        fprintf(logger, "[%d]Writer (%ld) entered at place %ld and is blocked by %ld reader(s) & %ld writer(s)\n",
            pid, place+1, *empty_space, blocked_by_r, blocked_by_w);
        fflush(logger);
    }

    sem_post(&(shm->mx));
    if (shm->writers[*empty_space].blocked_by != 0)
        sem_wait(&(shm->writers[*empty_space].sem));

    sem_wait(&(shm->mx));
    if (log_lvl > SIMPLE_LOG)
    {
        fprintf(logger, "[%d]Writer (%ld) passed the semaphore\n", pid, place+1);
        fflush(logger);
    }
    else
    {
        fprintf(logger, "[%d]Writer (%ld) entered\n", pid, place+1);
        fflush(logger);
    }

    if (log_lvl > MED_LOG)
    {
        print_active_readers(shm, logger);
        print_active_writers(shm, logger);
    }

    sem_post(&(shm->mx));
}

void find_blocking_writers(shm_buffer shm, const size_t index, const size_t place, FILE *logger, const log_type log_lvl)
{
    size_t unblocked_r = 0, unblocked_w = 0, readers_rem = 0, writers_rem = 0;

    sem_wait(&(shm->mx));

    shm->writers[index].type = EMPTY;
    shm->curr_writers--;

    const size_t my_ticket = shm->writers[index].ticket;

    for (size_t i = 0; i < PID_ARR_SIZE; i++)
    {
        // blocking reader
        if (shm->readers[i].type == OCCUPIED)
        {
            if (shm->readers[i].ticket > my_ticket && (place >= shm->readers[i].start && place <= shm->readers[i].end))
            {
                assert(shm->readers[i].blocked_by > 0);
                if (log_lvl > MED_LOG)
                {
                    fprintf(logger, "[%d]Writer (%ld) was blocking [%d]Reader\n", shm->writers[index].pid, shm->writers[index].start+1, shm->readers[i].pid);
                    fflush(logger);
                }
                
                unblocked_r++;

                // reader is not being blocked by any writer, wake him up
                if ((--(shm->readers[i].blocked_by)) == 0)
                    sem_post(&(shm->readers[i].sem));
            }
            readers_rem++;
        }
        // blocking writer
        if (shm->writers[i].type == OCCUPIED)
        {
            if (shm->writers[i].ticket > my_ticket && shm->writers[i].start == place)
            {
                assert(shm->writers[i].blocked_by > 0);

                if (log_lvl > MED_LOG)
                {
                    fprintf(logger, "[%d]Writer (%ld) was blocking [%d]Writer\n", shm->writers[index].pid, shm->writers[index].start+1, shm->writers[i].pid);
                    fflush(logger);
                }

                unblocked_w++;
                // writer is not being blocked by any process, wake him up
                if ((--(shm->writers[i].blocked_by)) == 0)
                    sem_post(&(shm->writers[i].sem));
            }
            writers_rem++;
        }
        if (readers_rem == shm->curr_readers && writers_rem == shm->curr_writers) break;
    }

    if (log_lvl > SIMPLE_LOG)
    {
        fprintf(logger, "[%d]Writer (%ld) exited after unblocking %ld reader(s) & %ld writer(s)\n", shm->writers[index].pid, place+1, unblocked_r, unblocked_w);
        fflush(logger);
    }
    else
    {
        fprintf(logger, "[%d]Writer (%ld) exited\n", shm->writers[index].pid, place+1);
        fflush(logger);
    }

    shm->curr_size--;

    if (log_lvl > MED_LOG)
    {
        print_active_readers(shm, logger);
        print_active_writers(shm, logger);
    }

    sem_post(&(shm->mx));
}
