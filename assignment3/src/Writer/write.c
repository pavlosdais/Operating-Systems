#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "../../include/write_utilities.h"
#include "../../include/utilities.h"

// format:
// -f <filename> -l <recid> -v <value> -d <time> -s <shmid>
int main(int argc, char* argv[])
{
    // get info from the command line arguments
    char* file_name;
    int recid, update_value, sleep_time;
    char* shared_mem;
    log_type log_lvl = SIMPLE_LOG;

    if (!open_cla_writer(argc, argv, &file_name, &recid, &update_value, &sleep_time, &shared_mem, &log_lvl))
    {
        fprintf(stderr, "Error! Usage: %s -f <filename> -l <recid> -v <value> -d <time> -s <shmid> -lg <log_level>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct timeval start, end;
    size_t index;
    struct _record record;

    // open the logger
    FILE* logger = fopen(LOG_FILE, "a");
    if (logger == NULL)
    {
        fprintf(stderr, "Error while trying to open the logger\n");
        exit(EXIT_FAILURE);
    }

    const pid_t process_id = getpid();
    const int fd = open_file(file_name, WRITE_PERM);

    // open shared memory segment
    shm_buffer shm;
    SAFE_SHARED_MEMORY(shm, shared_mem, "write.c")

    // start timer - see how long it takes to reach the critical section
    GET_TIME(start);

    sem_wait(&(shm->max_entered));

    // find the readers and/or writers that block us from advancing
    find_blocked_writers(shm, recid, process_id, &index, logger, log_lvl);
    
    srand(time(NULL));

    // end timer - reached CS
    GET_TIME(end);

    // do the writing
    lseek(fd, recid * sizeof(struct _record), SEEK_SET);

    // get the record
    SAFE_READ(fd, record, "write.c")

    // change the balace of the record
    lseek(fd, recid * sizeof(struct _record), SEEK_SET);
    record.balance += update_value;

    // write back the result
    SAFE_WRITE(fd, record, "write.c")

    sleep(1+(rand() % sleep_time));

    // find the readers and/or writers the writer was blocking from advancing & unblock them
    find_blocking_writers(shm, index, recid, logger, log_lvl);

    sem_post(&(shm->max_entered));

    // close logger
    fclose(logger);

    // Writing done, clean up
    long time_taken;
    CALC_TIME(start, end, time_taken);
    // printf("W %ld\n", time_taken);

    sem_wait(&(shm->update_w));

        shm->total_writers++;
        shm->write_time += time_taken;

    sem_post(&(shm->update_w));

    sem_wait(&(shm->update_r));
    sem_wait(&(shm->update_w));
    
        if (time_taken > shm->max_stall) shm->max_stall = time_taken;
        shm->num_processed++;

    sem_post(&(shm->update_w));
    sem_post(&(shm->update_r));

    close(fd);
    exit(EXIT_SUCCESS);
}
