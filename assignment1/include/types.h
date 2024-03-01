#pragma once

// input buffer sizes
#define BUFFER_SIZE 800
#define LINE_SIZE 800

// default starting capacity of the hash table
#define DEFAULT_ST_CAPACITY 2

// default number of elements a bucket can fit on the hash table
#define DEFAULT_BUCKET_SIZE 2

// default expand function of the hash table
#define DEFAULT_EXPAND_FUNC 1


typedef struct _linear_hash* hash_table;
typedef struct listSet* List;

struct _voter
{
    int PIN;        // pin
    char* name;     // name
    char* surname;  // surname
    int TK;         // postcode
    char voted;     // voted(y/n)
};
typedef struct _voter* voter;

struct _database
{
    hash_table ht;      // main data structure holding all participants and voters
    List list;          // list containing the voters
    size_t voters_num;  // the total number of participants
    int sorted;         // is the list with the zipcodes sorted
};
typedef struct _database* database;  // handle

// 1d list containing every zipcode along with every voter that resides in it
struct _postcode_info
{
    int postcode;  // postcode
    List voters;   // list of the voters with the specified postcode
};
typedef struct _postcode_info* postcode;

typedef enum
{
    FIND_PIN = 0,  // 1
    INSERT_HASH,   // 2
    VOTED,         // 3
    VOTED_FILE,    // 4
    VOTER_NUM,     // 5
    VOTER_PER,     // 6
    ZIP_NUM ,      // 7
    TK_VOTERS,     // 8
    EXIT,          // 9
    PRINT,         // 10 - mine
    UNRECOGNIZED
}
command_t;
