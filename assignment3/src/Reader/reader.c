#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "../../include/reader_utilities.h"
#include "../../include/utilities.h"

// format:
// -f <filename> -l <recid[,recid]> -d <time> -s <shmid>
int main(int argc, char* argv[])
{
    char* file_name;
    int recid_st, recid_end, sleep_time;
    char* shared_mem;
    log_type log_lvl;

    // get command line arguments
    if (!open_cla_reader(argc, argv, &file_name, &recid_st, &recid_end, &sleep_time, &shared_mem, &log_lvl))
    {
        fprintf(stderr, "Error! Usage: %s -f <filename> -l <recid_st> <recid_end> -d <time> -s <shmid> -lg <log_level>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    struct timeval start, end;
    size_t index;

    const int rec_read = recid_end - recid_st + 1;
    Record rec_array = custom_malloc(rec_read * sizeof(*rec_array));
    const int fd = open_file(file_name, READ_PERM);

    // open the logger
    FILE* logger = fopen(LOG_FILE, "a");
    if (logger == NULL)
    {
        fprintf(stderr, "Error while trying to open the logger\n");
        exit(EXIT_FAILURE);
    }

    const pid_t process_id = getpid();

    // open shared memory segment
    shm_buffer shm;
    SAFE_SHARED_MEMORY(shm, shared_mem, "reader.c")

    // start timer - see how long it takes to reach the critical section
    GET_TIME(start);

    sem_wait(&(shm->max_entered));
    
    // find the writers blocking the reader
    find_blocked_readers(shm, recid_st, recid_end, process_id, &index, logger, log_lvl);
    
    srand(time(NULL));

    // open file and go to the place
    lseek(fd, recid_st * sizeof(struct _record), SEEK_SET);

    // end timer - reached CS
    GET_TIME(end);

    // do the reading
    // read the records all at once in an array
    safe_read(rec_array, fd, rec_read * sizeof(*rec_array));
    
    // read operation
    sem_wait(&(shm->read_sem));
    for (int curr_record_id = 0; curr_record_id < rec_read; curr_record_id++)
    {
        // read record
        read_record(process_id, rec_array[curr_record_id]);
        continue;
    }
    sem_post(&(shm->read_sem));
    sleep(1+(rand() % sleep_time));

    // find the writers the reader was blocking and unblock them
    find_blocking_readers(shm, index, recid_st, recid_end, logger, log_lvl);
    
    // reading done, clean up
    sem_post(&(shm->max_entered));

    // close logger
    fclose(logger);

    free(rec_array);

    long time_taken;
    CALC_TIME(start, end, time_taken);

    sem_wait(&(shm->update_r));

        shm->total_readers++;
        shm->read_time += time_taken;

    sem_post(&(shm->update_r));

    sem_wait(&(shm->update_r));
    sem_wait(&(shm->update_w));

        if (time_taken > shm->max_stall) shm->max_stall = time_taken;
        shm->num_processed += rec_read;

    sem_post(&(shm->update_w));
    sem_post(&(shm->update_r));

    close(fd);
    exit(EXIT_SUCCESS);
}
