#include <unistd.h>
#include <stdbool.h>
#include <sys/times.h>
#include <signal.h>
#include "../include/utilities.h"
#include "../include/common.h"

static void heapify(const Record array, const int size, const int curr)
{
    // get left & right child
    const int left = 2*curr+1, right = 2*curr+2;
    
    // find the current max
    int curr_max = curr;
    if (left < size && compare_records(&array[left], &array[curr]) > 0) curr_max = left;

    if (right < size && compare_records(&array[right], &array[curr_max]) > 0) curr_max = right;
    
    // largest is not the root
    // heapify the sub-tree
    if (curr_max != curr)
    {
        swap_nodes(&array[curr], &array[curr_max]);
        heapify(array, size, curr_max);
    }
}

static inline void build_heap(const Record array, const int size)
{
    for (int i = size/2-1; i > -1; i--)
        heapify(array, size, i);
}

// source:
// https://en.wikipedia.org/wiki/Heapsort#Pseudocode
// https://www.youtube.com/watch?v=2DmK_H7IdTo
void heap_sort(const Record array, const int size)
{
    // build our heap
    build_heap(array, size);

    // each time reduce heap by one
    for (int i = size-1; i >= 0; i--)
    {
        swap_nodes(&array[0], &array[i]);
        heapify(array, i, 0);
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
    heap_sort(record_array, range);

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
