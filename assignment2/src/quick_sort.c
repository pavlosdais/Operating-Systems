#include <unistd.h>
#include <stdbool.h>
#include <sys/times.h>
#include <signal.h>
#include "../include/utilities.h"
#include "../include/common.h"

static inline int partition(Record array, const int left, const int right)
{
    struct _record pivot = array[right];  // choose the pivot

    // the right position of pivot found at moment
    int i = left-1;

    for (int j = left; j < right; j++)
    {
        if (compare_records(&array[j], &pivot) < 0)
        {
            // element smaller than the pivot is found
            // swap it with the larger element pointed by i
            swap_nodes(&array[++i], &array[j]);
        }
    }
    swap_nodes(&array[i+1], &array[right]);

    // we now return the partition where partition is done 
    return i+1;
}

// source:
// https://en.wikipedia.org/wiki/Quicksort#Algorithm
// https://www.youtube.com/watch?v=Hoixgm4-P4M
static void quick_sort(Record array, const int start, const int end)
{
    if (start < end)
    {
        // partition return index
        const int p = partition(array, start, end);

        // separately sort elements
        quick_sort(array, start, p-1);
        quick_sort(array, p+1, end);
    }
}

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Wrong number of command line arguments in heap sort..\n");
        exit(EXIT_FAILURE);
    }

    double t1, t2, cpu_time;
    struct tms tb1, tb2;
    double ticspersec;
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    t1 = (double) times (&tb1);

    // 1. get the file name
    const char* file_name = argv[1];

    // 2. start range
    const size_t start_r = atoi(argv[2]);

    // 3. end range
    const size_t end_r = atoi(argv[3]);
    
    // 4. the pipe we will be writing the sorted records at
    const int w_pipe = atoi(argv[4]);

    // 5. coordinator id
    const int coordinator_pid = atoi(argv[5]);

    // [start_r, end_r] contains the range the sorter will try to sort
    const size_t range = end_r-start_r+1;

    // print the range of the sorter for debugging purposes
    // printf("[%ld, %ld]\n", start_r, end_r);

    // create a record array that will hold the elements in the array
    const Record record_array = custom_malloc(range * sizeof(*record_array));

    // open the file and move the seek pointer to the start of the range we want
    int fd = open_file(file_name);
    lseek(fd, start_r * sizeof(struct _record), SEEK_SET);

    // read records from the file and store them in the array
    safe_read(record_array, fd, range*sizeof(struct _record));

    // records read, close file descriptor
    close(fd);

    // sort the array
    quick_sort(record_array, 0, range-1);

    // calculate run time & cpu time
    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));
    calculated_time calc_time;
    calc_time.cpu_time = cpu_time / ticspersec;
    calc_time.run_time = (t2 - t1)/ticspersec;

    // pass back the info to the splitter
    write(w_pipe, record_array, range * sizeof(struct _record));
    write(w_pipe, &calc_time, sizeof(calculated_time));
    
    // send SIGUSR2 signal to the coordinator that splitter has finished
    kill(coordinator_pid, SIGUSR2);

    free(record_array);
    exit(EXIT_SUCCESS);
}
