#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/poll.h>
#include "../include/common.h"
#include "../include/signal_handler.h"
#include "../include/utilities.h"

// get the external variables from the signal_handler
volatile sig_atomic_t signals_arrived_sorters = 0;
volatile sig_atomic_t signals_arrived_splitters = 0;

int main(int argc, char* argv[])
{
    // get command line aruments
    char* file_name = NULL;
    size_t num_of_children;
    char* sort1 = NULL;
    char* sort2 = NULL;
    if (!open_cla_coordinator(argc, argv, &file_name, &num_of_children, &sort1, &sort2))
    {
        fprintf(stderr, "Error! Usage %s -i <data_file> -k <number_of_children> -e1 sorting1 -e2 sorting2\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // the number of records the file holds
    const size_t file_size = records_size(file_name);

    // pipes for collecting the records
    int** record_pipes = create_pipes(num_of_children);

    // create the ranges the splitter will take on to sort
    Range* splitter_ranges = calculate_splitter_range(file_size, num_of_children);

    // where to save the records taken from the splitter
    Record* record_array = create_record_arr(num_of_children, splitter_ranges);

    // where to save the time needed by our sorters
    calculated_time** results_cpu = custom_malloc(num_of_children * sizeof(*results_cpu));
    for (size_t splitter_num = 0; splitter_num < num_of_children; splitter_num++)
        results_cpu[splitter_num] = custom_calloc((num_of_children - splitter_num), sizeof(*results_cpu[splitter_num]));

    // create poll array
    struct pollfd* fds = create_poll_arrays(record_pipes, num_of_children);

    // create sigaction for splitters
    // splitters will be sending SIGUSR1 signal, signaling they have finished
    struct sigaction signal_action_splitters;
    CREATE_SIGNAL_ACTION(signal_action_splitters, signal_handler_splitters, SIGUSR1)

    // create sigaction for sorters
    // sorters will be sending SIGUSR2 signal, signaling they have finished
    struct sigaction signal_action_sorters;
    CREATE_SIGNAL_ACTION(signal_action_sorters, signal_handler_sorters, SIGUSR2)

    // create the second level // splitters
    int child_pid[num_of_children];
    for (size_t splitter_num = 0; splitter_num < num_of_children; splitter_num++)
    {
        // create splitter <splitter_num>
        SAFE_FORK(child_pid[splitter_num], "coordinator.c");

        if (child_pid[splitter_num] == 0)  // child process - at splitter
        {
            // now we need to format the data to send it to the splitter
            close(record_pipes[splitter_num][PIPE_READ]);

            // 2. send the total number of children
            char* created_sorters = int_to_string(num_of_children - splitter_num);

            // 3. start range
            char* start_p = int_to_string(splitter_ranges[splitter_num]->start);

            // 4. end range
            char* end_p = int_to_string(splitter_ranges[splitter_num]->end);

            // 5. send write signal for the records to pass
            char* record_signal = int_to_string(record_pipes[splitter_num][PIPE_WRITE]);

            // format the arguments
            char* arguments[] = { SPLITTER_EXEC, file_name, created_sorters, start_p, end_p, record_signal, sort1, sort2, NULL };
            SAFE_EXECVP(arguments, "coordinator.c")
        }
        else close(record_pipes[splitter_num][PIPE_WRITE]);
    }

    size_t nfds_read = 0;
    while (nfds_read < num_of_children)
    {
        // wait for a splitter to write
        int ret;
        SAFE_POLL(fds, num_of_children, ret, "coordinator.c")
        // a child wrote
        if (ret > 0)
        {
            // find the splitter that sent records through the pipe
            for (size_t i = 0; i < num_of_children; i++)
            {
                if (fds[i].revents & POLLIN)
                {
                    // read records
                    safe_read(record_array[i], record_pipes[i][PIPE_READ], splitter_ranges[i]->range * sizeof(struct _record));

                    // read times
                    safe_read(results_cpu[i], record_pipes[i][PIPE_READ], (num_of_children-i) * sizeof(calculated_time));

                    nfds_read++;  // keep incrementing the number of children read, until we get all of them
                }
            }
        }
    }

    // wait for the splitters to finish
    int return_status;
    while (wait(&return_status) > 0);

    // print the sorted records
    print_merged(record_array, splitter_ranges, num_of_children);

    // print the time spent by each sorter
    print_times(results_cpu, num_of_children);

    // print the signals arrived
    printf("Got %d signals from sorters\n", signals_arrived_sorters);
    printf("Got %d signals from splitters\n", signals_arrived_splitters);

    // destroy memory used by the program
    for (size_t splitter_num = 0; splitter_num < num_of_children; splitter_num++)
    {
        free(record_array[splitter_num]);
        free(results_cpu[splitter_num]);
        destroy_range(splitter_ranges[splitter_num]);
    }

    free(sort1);
    free(sort2);
    free(fds);
    free(record_array);
    free(results_cpu);
    free(splitter_ranges);
        
    destroy_pipes(record_pipes, num_of_children);
    exit(EXIT_SUCCESS);
}
