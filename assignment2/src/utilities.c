#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/poll.h>
#include "../include/utilities.h"
#include "../include/common.h"
#include "../include/signal_handler.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General use functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int** create_pipes(const size_t size)
{
    int** pipes = custom_malloc(size * sizeof(int*));

    for (size_t i = 0; i < size; i++)
    {
        // create & allocate memory for a pipe
        pipes[i] = custom_malloc(2 * sizeof(int));
        if ((pipe(pipes[i])) == -1)
        {
            fprintf(stderr, "Error while trying to create a pipe\n");
            exit(EXIT_FAILURE);
        }
    }
    return pipes;
}

void destroy_pipes(int** pipes, const size_t size)
{
    for (size_t i = 0; i < size; i++)
        free(pipes[i]);
    free(pipes);
}

char* int_to_string(int num)
{
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%d", num);
    
    char *result = custom_calloc(sizeof(buffer), sizeof(char));
    strcpy(result, buffer);

    return result;
}

char* alloc_n_cpy(char* buffer, const size_t size)
{
    char* new_string = custom_malloc(size * sizeof(char));
    strcpy(new_string, buffer);
    return new_string;
}

Record* create_record_array(const size_t size)
{
    Record* record_arr = custom_malloc(size * sizeof(*record_arr));
    for (size_t i = 0; i < size; i++)
        record_arr[i] = custom_calloc(1, sizeof(struct _record));
    return record_arr;
}

int compare_records(const Record a, const Record b)
{
    const int cmp_surnames = strcmp(a->surname, b->surname);  // compare surnames
    if (cmp_surnames == 0)  // surnames are the same, compare names
    {
        int cmp_names = strcmp(a->name, b->name);
        if (cmp_names == 0)  // surnames are the same, compare AM
            return a->AM - b->AM;
        else if (cmp_names < 0) return -1;
        return 1;
    }
    else if (cmp_surnames < 0) return -1;
    return 1;
}

void destroy_records(Record* record_arr, const size_t size)
{
    for (size_t i = 0; i < size; i++)
        free(record_arr[i]);
    
    free(record_arr);
}

Range create_range(const size_t start, const size_t end)
{
    const Range range = custom_malloc(sizeof(*range));
    range->start = start;
    range->end = end;
    range->range = end-start+1;
    return range;
}

void destroy_range(const Range range)  { free(range); }

Record merge_records(Record* record_array, Range* ranges, const size_t array_num, size_t* final_size)
{
    // the size of our merged array
    *final_size = 0;
    for (size_t i = 0; i < array_num; i++) *final_size += ranges[i]->range;

    // create the merged array
    Record merged_array = custom_malloc(*final_size * sizeof(struct _record));
    
    // create an array of indices containing the index of each record array
    size_t* curr_index = custom_calloc(array_num, sizeof(size_t));

    for (size_t i = 0; i < *final_size; i++)
    {
        size_t k;
        struct _record max_record;
        for (k = 0; k < array_num; k++)
        {
            if (curr_index[k] != ranges[k]->range)
            {
                memcpy(&max_record, &record_array[k][curr_index[k]], sizeof(struct _record));
                break;
            }
        }
        if (k == array_num) break;

        size_t max_index = k;

        // search every array for the lowest value
        for (size_t j = k; j < array_num; j++)
        {
            const size_t curr_arr_index = curr_index[j];

            // check if the index of the array is in range and if it has a lower priority with our current max record
            if (curr_arr_index != ranges[j]->range && compare_records(&record_array[j][curr_arr_index], &max_record) < 0)
            {
                memcpy(&max_record, &record_array[j][curr_arr_index], sizeof(struct _record));
                max_index = j;
            }
        }

        memcpy(&merged_array[i], &max_record, sizeof(struct _record));

        curr_index[max_index]++;
    }
    free(curr_index);
    return merged_array;
}

void safe_read(void* source, const int pipe_num, size_t read_size)
{
    ssize_t bytes_read = 1;
    size_t total_bytes_read = 0;
    while (bytes_read != 0)
    {
        // read from pipe
        bytes_read = read(pipe_num, (char*)source+total_bytes_read, read_size);
        if (bytes_read  < 0)
        {
            perror("Error at safe read\n");
            exit(EXIT_FAILURE);
        }

        // increment the number of bytes read
        total_bytes_read += bytes_read;

        // decrement the number of bytes we still need to read 
        read_size -= bytes_read;
    }
}

void print_record(const Record record)
{
    printf("%-12s %-12s %-6d %s\n", record->surname, record->name,  record->AM, record->zipcode);
    // printf("%d %s %s\n", record->AM, record->surname, record->name);
}

Record* create_record_arr(const size_t size, Range* ranges)
{
    Record* record_arr = custom_malloc(size * sizeof(*record_arr));
    for (size_t i = 0; i < size; i++) record_arr[i] = custom_calloc(ranges[i]->range, sizeof(struct _record));
    return record_arr;
}

struct pollfd* create_poll_arrays(int** pipes, const size_t number)
{
    struct pollfd* fds = custom_malloc(number*sizeof(struct pollfd));
    for (size_t i = 0; i < number; i++)
    {
        fds[i].fd = pipes[i][PIPE_READ];
        fds[i].events = POLLIN;
    }
    return fds;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Coordinator functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int string_to_int(const char *str)
{
    char *end_ptr;
    const long num = strtol(str, &end_ptr, 10);

    // invalid format was given, return -1
    if (*end_ptr != '\0' && *end_ptr != '\n') return -1;
    return (int)num;
}

bool open_cla_coordinator(int argc, char* argv[], char** file_name, size_t* num_of_children, char** sort1, char** sort2)
{
    if (argc % 2 == 0 || argc > 9) return false;

    // look for the correct commandline arguments
    *num_of_children = 0;
    *sort1 = NULL;
    *sort2 = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (i == argc-1) continue;

            if (strlen(argv[i]) == 2)
            {
                if (argv[i][1] == 'i')  // -i <data_file>
                    *file_name = argv[i+1];
                else if (argv[i][1] == 'k')  // -k <num_of_children>
                    *num_of_children = string_to_int(argv[i+1]);
            }
            else if (strlen(argv[i]) == 3 && argv[i][1] == 'e')  // -e <sorting>
            {
                if (argv[i][2] == '1')  // sorting1
                    *sort1 = alloc_n_cpy(argv[i+1], strlen(argv[i+1])+1);
                else if (argv[i][2] == '2')  // sorting2
                    *sort2 = alloc_n_cpy(argv[i+1], strlen(argv[i+1])+1);
            }
        }
    }

    // a file name must be given
    if (*file_name == NULL) return false;

    // the number of chir was not give, use the default number
    if (*num_of_children == 0) *num_of_children = DEFAULT_NUMBER_CHILDREN;
    
    // no sort function was given for sort1, by default use quick sort
    if (*sort1 == NULL) *sort1  = alloc_n_cpy(QUICKSORT_EXEC, strlen(QUICKSORT_EXEC)+1);

    // no sort function was given for sort2, by default use quick sort
    if (*sort2 == NULL) *sort2  = alloc_n_cpy(HEAPSORT_EXEC, strlen(HEAPSORT_EXEC)+1);

    return true;
}

size_t records_size(const char* file_name)
{
    const int fd = open_file(file_name);  // open file
    const size_t file_size = lseek(fd, 0, SEEK_END)/sizeof(struct _record);  // calculate number of records
    close(fd);  // close file
    return file_size;
}

Range* calculate_splitter_range(const size_t file_size, const size_t num_of_children)
{
    Range* splitter_ranges = custom_malloc(num_of_children * sizeof(*splitter_ranges));

    const size_t sort_size = file_size/num_of_children;

    const size_t err = file_size % num_of_children;
    for (size_t splitter_num = 0; splitter_num < num_of_children; splitter_num++)
    {
        // get the range the splitter will take on to sort 
        const size_t start = splitter_num * sort_size;
        size_t end         = (splitter_num + 1) * sort_size-1;
        
        if (err != 0 && splitter_num == num_of_children-1) end += err;

        splitter_ranges[splitter_num] = create_range(start, end);
    }
    return splitter_ranges;
}

void print_times(calculated_time** results_cpu, const size_t num_of_children)
{
    // print the time needed for each sorter
    for (size_t splitter_num = 0; splitter_num < num_of_children; splitter_num++)
    {
        printf("Splitter %ld\n", splitter_num);
        for (size_t sorter_num = 0; sorter_num < num_of_children - splitter_num; sorter_num++)
            printf("Sorter %ld| cpu time = %lf | run time = %lf\n", sorter_num,
                                                                    results_cpu[splitter_num][sorter_num].cpu_time,
                                                                    results_cpu[splitter_num][sorter_num].run_time);
    }
}

void print_merged(Record* record_array, Range* ranges, const size_t array_num)
{
    size_t final_size = 0;
    for (size_t i = 0; i < array_num; i++) final_size += ranges[i]->range;
    size_t* curr_index = custom_calloc(array_num, sizeof(size_t));
    for (size_t i = 0; i < final_size; i++)
    {
        size_t k;
        struct _record max_record;
        for (k = 0; k < array_num; k++)
        {
            if (curr_index[k] != ranges[k]->range)
            {
                memcpy(&max_record, &record_array[k][curr_index[k]], sizeof(struct _record));
                break;
            }
        }
        if (k == array_num) break;
        size_t max_index = k;
        for (size_t j = k; j < array_num; j++)
        {
            const size_t curr_arr_index = curr_index[j];

            if (curr_arr_index != ranges[j]->range && compare_records(&record_array[j][curr_arr_index], &max_record) < 0)
            {
                memcpy(&max_record, &record_array[j][curr_arr_index], sizeof(struct _record));
                max_index = j;
            }
        }
        print_record(&max_record);
        curr_index[max_index]++;
    }
    free(curr_index);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Splitter functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Range* calculate_sorter_range(const size_t start, const size_t end, const size_t total_splitters)
{
    const size_t sort_size = end-start;
    const size_t sorter_size = sort_size/total_splitters;
    const size_t rem = sort_size % total_splitters;

    Range* sorter_ranges = custom_malloc(total_splitters * sizeof(*sorter_ranges));
    for (size_t sorter_num = 0; sorter_num < total_splitters; sorter_num++)
    {
        const size_t offset1 = sorter_num * sorter_size;
        const size_t offset2 = offset1 + sorter_size;

        size_t start_r = start+offset1;
        size_t end_r = offset2+start-1;

        // add adjustment
        if (rem != 0 && sorter_num+1 == total_splitters) end_r += rem;
        
        sorter_ranges[sorter_num] = create_range(start_r, end_r);
    }
    return sorter_ranges;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sorter functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void swap_nodes(struct _record* a, struct _record* b)
{
    struct _record tmp;
    memcpy(&tmp, a, sizeof(struct _record));
    memcpy(a, b, sizeof(struct _record));
    memcpy(b, &tmp, sizeof(struct _record));
}

int open_file(const char* file_name)
{
    // try to open the file
    int fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        perror("Error while trying to open file, exiting\n");
        exit(EXIT_FAILURE);
    }
    return fd;
}
