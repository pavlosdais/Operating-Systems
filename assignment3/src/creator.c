#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/time.h>
#include "../include/common.h"
#include "../include/utilities.h"

// format:
// -sm <shared_memory> -f <filename> -rn <readers_num> -wn <writers_num> -rt <readers_time> -wt <writer_time> -wp <writer_path> -rp <reader_path>
int main(int argc, char* argv[])
{
    // get command line arguments
    create_info readers, writers;
    char* file_name; char* shared_mem; char* log_lvl;

    if (!open_cla_creator(argc, argv, &file_name, &shared_mem, &readers, &writers, &log_lvl))
    {
        fprintf(stderr, "Error! Usage: %s -rn <readers> -wn <writers> -rt <rtime> -wt <wtime> -wp <wpath> -rp <rpath> -lg <log_level>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // calculate the number of records the file currently holds
    const size_t num_of_records = records_size(file_name);

    // step 1: initialize shared memory + semaphores
    shm_buffer shm = shm_create(shared_mem);
    if (shm == NULL)
    {
        fprintf(stderr, "Error! Shared memory segment can not be opened\n");
        exit(EXIT_FAILURE);
    }
    
    time_t t;
    srand((unsigned) time(&t));

    // step 2: deploy readers & writers
    struct timeval start, end;
    GET_TIME(start);

    for (size_t i = 0, total_num = readers.numbers + writers.numbers; i < total_num; i++)
    {
        // pick randomly if we'll deploy reader/ writer
        int choice;
        if (readers.numbers == 0)      choice = WRITER;
        else if (writers.numbers == 0) choice = READER;
        else                           choice = rand() & 1;
        
        // we picked reader: get range [start, end] of records to read
        // we picked writer: get which record to write (start), and what the updated value will be (end)
        int start, end;
        if (choice == READER)
        {
            get_random_range(&start, &end, num_of_records);
            readers.numbers--;
        }
        else
        {
            start = get_random_record(num_of_records);
            end   = get_random_value(MAX_UPDATE_AMMOUNT);
            writers.numbers--;
        }

        // deploy process
        pid_t pid;
        SAFE_FORK(pid, "creator.c", shm, shared_mem)
        if (pid == 0)  // child
        {
            char* recid_start = int_to_string(start);

            if (choice == READER)  // reader
            {
                char* recid_end   = int_to_string(end);
                char* arguments[] = { readers.path, "-f", file_name, "-l", recid_start, recid_end, "-d", readers.time, "-s", shared_mem, "-lg", log_lvl, NULL };
                SAFE_EXECVP(arguments, "reader.c", shm, shared_mem)
            }
            else  // writer
            {
                char* tmp_value   = int_to_string(end);
                char* arguments[] = { writers.path, "-f", file_name, "-l", recid_start, "-v", tmp_value, "-d", writers.time, "-s", shared_mem, "-lg", log_lvl, NULL };
                SAFE_EXECVP(arguments, "write.c", shm, shared_mem)
            }
        }
        else continue;
    }

    // wait for all readers & writers to end
    while (wait(NULL) > 0);

    // step 3: report statistics
    GET_TIME(end);
    long time_taken;
    CALC_TIME(start, end, time_taken);

    // print different stastics
    show_statistics(shm);
    printf("7. Total time taken                = %.4f seconds\n", (float)time_taken/ MICRO_T);
    
    // unlink shared memory
    shm_destroy(shm, shared_mem);

    exit(EXIT_SUCCESS);
}
