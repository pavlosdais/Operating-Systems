#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include "../include/common.h"
#include "../include/utilities.h"

char* alloc_n_cpy(char* buffer, const size_t size)
{
    char* new_string = custom_malloc((size+1) * sizeof(char));
    strcpy(new_string, buffer);
    return new_string;
}

char* int_to_string(int num)
{
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%d", num);
    
    char *result = custom_calloc(sizeof(buffer), sizeof(char));
    strcpy(result, buffer);

    return result;
}

bool open_cla_creator(int argc, char* argv[], char** file_name, char** shared_mem, create_info* readers, create_info* writers, char** log_level)
{
    if (argc != 19) return false;
        
    *file_name = NULL;
    for (int i = 1; i < argc-1; i+=2)
    {
        if (strlen(argv[i]) == 3)
        {
            if (strcmp(argv[i], "-rn") == 0)       // -rn <readers_num>
                readers->numbers = atoi(argv[i+1]);
            else if (strcmp(argv[i], "-wn") == 0)  // -wn <writers_num>
                writers->numbers = atoi(argv[i+1]);
            else if (strcmp(argv[i], "-rt") == 0)  // -rt <readers_time>
                readers->time = argv[i+1];
            else if (strcmp(argv[i], "-wt") == 0)  // -wt <writers_time>
                writers->time = argv[i+1];
            else if (strcmp(argv[i], "-rp") == 0)  // -rp <reader_path>
                readers->path = argv[i+1];
            else if (strcmp(argv[i], "-wp") == 0)  // -wp <writer_path>
                writers->path = argv[i+1];
            else if (strcmp(argv[i], "-sm") == 0)  // -sm
                *shared_mem = argv[i+1];
            else if (strcmp(argv[i], "-lg") == 0)
                *log_level = argv[i+1];
            else return false;
        }
        else if (strcmp(argv[i], "-f") == 0)
            *file_name = argv[i+1];
        else return false;
    }

    return true;
}

shm_buffer open_shm(const char* shmpath)
{
    const int fd = shm_open(shmpath, O_RDWR, 0);
    if (fd == ERROR_CODE)
        return NULL;

    shm_buffer shm = mmap(NULL, sizeof(*shm), COPE_MMAP, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED)
        return NULL;
    
    close(fd);
    return shm;
}

bool init_semaphore(sem_t* sem, const int value)
{
    return sem_init(sem, 1, value) != ERROR_CODE;
}

shm_buffer shm_create(const char* shmpath)
{
    const int fd = shm_open(shmpath, CODE_SHMOPEN, 0600);
    if (fd == ERROR_CODE)
        return NULL;

    if (ftruncate(fd, sizeof(struct _shmbuf)) == ERROR_CODE)
        return NULL;

    shm_buffer shm = mmap(NULL, sizeof(*shm), COPE_MMAP, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED)
        return NULL;

    // initialize semaphores
    if (!init_semaphore(&(shm->in), 1))  exit(EXIT_FAILURE);
    
    if (!init_semaphore(&(shm->mx), 1))  exit(EXIT_FAILURE);
    
    if (!init_semaphore(&(shm->wrt), 1)) exit(EXIT_FAILURE);

    if (!init_semaphore(&(shm->update_r), 1)) exit(EXIT_FAILURE);
    
    if (!init_semaphore(&(shm->update_w), 1)) exit(EXIT_FAILURE);

    if (!init_semaphore(&(shm->read_sem), 1)) exit(EXIT_FAILURE);
    
    if (!init_semaphore(&(shm->max_entered), PID_ARR_SIZE)) exit(EXIT_FAILURE);
    
    for (size_t i = 0; i < PID_ARR_SIZE; i++)
    {
        shm->readers[i].type = shm->writers[i].type = EMPTY;

        if (!init_semaphore(&(shm->readers[i].sem), 0)) exit(EXIT_FAILURE);
        if (!init_semaphore(&(shm->writers[i].sem), 0)) exit(EXIT_FAILURE);
    }
    
    // initialize shared memory
    shm->curr_readers = shm->curr_writers = shm->num_processed = shm->curr_ticket = shm->curr_size = 0;
    shm->total_readers = shm->total_writers = 0;
    
    close(fd);
    return shm;
}

void print_active_readers(shm_buffer shm, FILE *logger)
{
    fprintf(logger, "Active readers:\n[ ");
    fflush(logger);

    char entered = 0;
    for (size_t i = 0; i < PID_ARR_SIZE; i++)
    {
        if (shm->readers[i].type == OCCUPIED)
        {
            if (shm->readers[i].blocked_by == 0)
            {
                entered = 1;
                fprintf(logger, "%d ", shm->readers[i].pid);
                fflush(logger);
            }   
        }
    }
    if (!entered)
    {
        fprintf(logger, "EMPTY ");
        fflush(logger);
    }
    fprintf(logger, "]\n");
    fflush(logger);
}

void print_active_writers(shm_buffer shm, FILE *logger)
{
    fprintf(logger, "Active writers:\n[ ");
    fflush(logger);

    char entered = 0;
    for (size_t i = 0; i < PID_ARR_SIZE; i++)
    {
        if (shm->writers[i].type == OCCUPIED)
        {
            if (shm->writers[i].blocked_by == 0)
            {
                entered = 1;
                fprintf(logger, "%d ", shm->writers[i].pid);
                fflush(logger);
            }
        }
    }
    if (!entered)
    {
        fprintf(logger, "EMPTY ");
        fflush(logger);
    }
    fprintf(logger, "]\n");
    fflush(logger);
}

int open_file(const char* file_name, const int permission)
{
    // try to open the file
    const int fd = open(file_name, permission);
    if (fd == -1)
    {
        fprintf(stderr, "Error while trying to open file %s, exiting\n", file_name);
        exit(EXIT_FAILURE);
    }
    return fd;
}

size_t records_size(const char* file_name)
{
    const int fd = open_file(file_name, READ_PERM);  // open file
    const size_t file_size = lseek(fd, 0, SEEK_END)/sizeof(struct _record);  // calculate number of records
    close(fd);  // close file
    return file_size;
}

void get_random_range(int* start, int* end, const int num_of_records)
{
    *start = get_random_record(num_of_records-1);
    do { *end = get_random_record(num_of_records); }
    while (*start > *end);
}

int get_random_record(const int num_of_records)
{
    return rand() % num_of_records;
}

int get_random_value(const int max_amount)
{
    int amount = rand() % max_amount;
    if (rand() % 3 == 0) amount = -amount;
    return amount;
}

void safe_read(void* source, const int fd, size_t read_size)
{
    ssize_t bytes_read = 1;
    size_t total_bytes_read = 0;
    while (bytes_read != 0)
    {
        // read from file descriptor
        bytes_read = read(fd, (char*)source+total_bytes_read, read_size);
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

void read_record(const pid_t process_id, const struct _record record)
{
    printf("%d| %d %s %s %d\n", process_id, record.id, record.fname, record.lname, record.balance);
    fflush(stdout);
}

void show_statistics(shm_buffer shm)
{
    printf("[Statistics]\n");
    printf("1. Number of readers               = %ld\n", shm->total_readers);
    printf("2. Average waiting time of readers = %.4f seconds\n", ((float)shm->read_time / shm->total_readers)/MICRO_T);
    printf("3. Number of writers               = %ld\n", shm->total_writers);
    printf("4. Average waiting time of writers = %.4f seconds\n", ((float)shm->write_time / shm->total_writers)/MICRO_T);
    printf("5. Max waiting time                = %.4f seconds\n", (float)shm->max_stall/MICRO_T);
    printf("6. Total number of operations      = %ld\n", shm->num_processed);
}

void shm_destroy(shm_buffer shm, const char* shmpath)
{
    sem_destroy(&(shm->in));
    sem_destroy(&(shm->mx));
    sem_destroy(&(shm->wrt));
    sem_destroy(&(shm->max_entered));
    sem_destroy(&(shm->update_r));
    sem_destroy(&(shm->update_w));
    sem_destroy(&(shm->read_sem));

    for (size_t i = 0; i < PID_ARR_SIZE; i++)
    {
        sem_destroy(&(shm->readers[i].sem));
        sem_destroy(&(shm->writers[i].sem));
    }

    shm_unlink(shmpath);
}
