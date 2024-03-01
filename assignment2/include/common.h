#pragma once
#include <stdio.h>

struct _record
{
    int AM;
    char surname[20];
    char name[20];
    char zipcode[6];
};
typedef struct _record* Record;

typedef struct _calc_time
{
    double cpu_time;  // the cpu time elapsed
    double run_time;  // the run time elapsed
}
calculated_time;

struct _range
{
    size_t start;  // the start
    size_t end;    // the end
    size_t range;  // the overall range
};
typedef struct _range* Range;

// the default number of children of the splitter, in case none are given
#define DEFAULT_NUMBER_CHILDREN 4

typedef enum
{
    SORT_NOT_GIVEN = -1,
    QUICKSORT,
    HEAPSORT
}
SORTING_FUNCTIONS;

// the max number of bytes pipes can send/receive
#define MAX_MESSAGE_LENGTH 50

// pipes: read at index 0, write at index 1
enum { PIPE_READ = 0, PIPE_WRITE };

#define SPLITTER_EXEC "./bin/splitter"
#define HEAPSORT_EXEC "./bin/heap_sort"
#define QUICKSORT_EXEC "./bin/quick_sort"
