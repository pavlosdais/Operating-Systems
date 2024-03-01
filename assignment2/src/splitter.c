#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/poll.h>
#include "../include/utilities.h"
#include "../include/common.h"

int main(int argc, char* argv[])
{
    if (argc != 8)
    {
        fprintf(stderr, "Wrong number of command line arguments in splitter..\n");
        exit(EXIT_FAILURE);
    }

    // 1. get the file name
    char* file_name = alloc_n_cpy(argv[1], strlen(argv[1])+1);

    // 2. the number of sorters we will deploy
    const size_t total_sorters_num = atoi(argv[2]);

    // 3. start range
    const size_t start = atoi(argv[3]);

    // 4. end range
    const size_t end = atoi(argv[4])+1;

    // 5. pipe to write the records
    const int record_pipe = atoi(argv[5]);

    // 6. Sorting function (1)
    char* sort_func1 = argv[6];

    // 7. Sorting function (2)
    char* sort_func2 = argv[7];

    // get coordinator's process id
    int coordinator_pid = getppid();
    
    // create the pipes our sorters will be passing us the sorted records to
    int** record_pipes = create_pipes(total_sorters_num);

    // create the ranges our sorters will be taking on
    Range* sorter_ranges = calculate_sorter_range(start, end, total_sorters_num);

    // where we will be saving the time needed for our sorters
    calculated_time* results_cpu = custom_malloc(total_sorters_num * sizeof(*results_cpu));

    // create a 2d array where we will store the records passed by the sorters in order to later merge them
    Record* record_array = create_record_arr(total_sorters_num, sorter_ranges);

    // create poll arrays
    struct pollfd* fds = create_poll_arrays(record_pipes, total_sorters_num);

    // create the third level // sorters
    int child_pid[total_sorters_num];
    for (size_t sorter_num = 0; sorter_num < total_sorters_num; sorter_num++)
    {
        // create sorter <curr_sort>
        SAFE_FORK(child_pid[sorter_num], "splitter.c");
        if (child_pid[sorter_num] == 0)  // child process - at splitter
        {
            close(record_pipes[sorter_num][PIPE_READ]);

            // 2. send the start range
            char* start_range_str = int_to_string(sorter_ranges[sorter_num]->start);

            // 3. send the end range
            char* end_range_str = int_to_string(sorter_ranges[sorter_num]->end);

            // 4. pass the pipe desc the child will be sending the sorted records to
            char* records_pipe = int_to_string(record_pipes[sorter_num][PIPE_WRITE]);

            // 5. pass the coordinator id
            char* coord_id = int_to_string(coordinator_pid);

            if (sorter_num % 2 == 0)
            {
                char* arguments[] = { sort_func1, file_name, start_range_str, end_range_str, records_pipe, coord_id, NULL };
                SAFE_EXECVP(arguments, "coordinator.c")
            }
            else
            {
                char* arguments[] = { sort_func2, file_name, start_range_str, end_range_str, records_pipe, coord_id, NULL };
                SAFE_EXECVP(arguments, "coordinator.c")
            }
        }
        else close(record_pipes[sorter_num][PIPE_WRITE]);
    }
    
    // read the results from the sorters we deployed
    size_t nfds_read = 0;
    while (nfds_read < total_sorters_num)
    {
        int ret;
        SAFE_POLL(fds, total_sorters_num, ret, "splitter.c")
        // a child wrote
        if (ret > 0)
        {
            // find the sorter that sent records through the pipe
            for (size_t i = 0; i < total_sorters_num; i++)
            {
                if (fds[i].revents & POLLIN)
                {
                    // get records
                    safe_read(record_array[i], record_pipes[i][PIPE_READ], sorter_ranges[i]->range * sizeof(struct _record));

                    // get sort times
                    safe_read(&results_cpu[i], record_pipes[i][PIPE_READ], sizeof(calculated_time));

                    nfds_read++;  // keep incrementing the number of children read, until we get all of them
                }
            }
        }
    }

    // wait for the sorters to finish
    int return_status;
    while (wait(&return_status) > 0);
    
    // merge the results
    size_t merged_size;
    Record merged_records = merge_records(record_array, sorter_ranges, total_sorters_num, &merged_size);
    
    // pass the info to the coordinator
    write(record_pipe, merged_records, merged_size * sizeof(struct _record));
    write(record_pipe, results_cpu, total_sorters_num * sizeof(calculated_time));

    // destroy memory used by the program
    for (size_t sorter_num = 0; sorter_num < total_sorters_num; sorter_num++)
    {
        free(record_array[sorter_num]);
        destroy_range(sorter_ranges[sorter_num]);
    }
    free(record_array);
    free(sorter_ranges);

    // destroy memory used for the pipes
    destroy_pipes(record_pipes, total_sorters_num);
    free(file_name);
    free(merged_records);
    free(results_cpu);
    free(fds);

    // send SIGUSR1 signal to the coordinator that splitter has finished
    kill(coordinator_pid, SIGUSR1);

    exit(EXIT_SUCCESS);
}
