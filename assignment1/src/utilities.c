#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "../include/database.h"
#include "../include/utilities.h"
#include "../include/list.h"

char command_num(char* ans)
{
    // convert command to lowercase to allow uppercase characters
    for (size_t i = 0; ans[i] != '\0'; i++)
        if (isupper(ans[i])) ans[i] = tolower(ans[i]);

    if (strcmp("l", ans) == 0) return FIND_PIN;
    else if (strcmp("i", ans) == 0) return INSERT_HASH;
    else if (strcmp("m", ans) == 0) return VOTED;
    else if (strcmp("bv", ans) == 0) return VOTED_FILE;
    else if (strcmp("v", ans) == 0) return VOTER_NUM;
    else if (strcmp("perc", ans) == 0) return VOTER_PER;
    else if (strcmp("z", ans) == 0) return ZIP_NUM;
    else if (strcmp("o", ans) == 0) return TK_VOTERS;
    else if (strcmp("exit", ans) == 0) return EXIT;
    else if (strcmp("p", ans) == 0) return PRINT;

    else return UNRECOGNIZED;  // command not recognized
}

bool check_malformed(const char* str)
{
    if (str == NULL)
    {
        unsuccessful_response("Malformed Input");
        return true;
    }
    return false;
}

char* mystrcpy(const char* src)
{
    // count the characters of the string
    const size_t num = strlen(src);

    char* result = custom_calloc(num+1, sizeof(char));
    result[num] = '\0';

    // copy
    for (size_t i = 0; i < num; i++) result[i] = src[i];
    return result;
}

void successful_response(const char* msg)
{
    fprintf(stdout, "%s\n\n", msg);
}

void unsuccessful_response(const char* msg)
{
    fprintf(stderr, "%s\n\n", msg);
}

void command_preprocess(char* buff)
{
    size_t i = 0, j = 0;
    char whsp = false;
    while (buff[i] != '\n' && buff[i] != '\0')
    {
        // start skipping whitespaces
        if (buff[i] == ' ' || buff[i] == '\t')
        {
            i++;
            whsp = true;  // enter white space mode
            continue;
        }
        if (whsp)
            buff[--i] = ' ';

        buff[j] = buff[i];
        
        i++;
        j++;
        whsp = 0;
    }
    // null-terminate the buffer
    buff[j] = '\0';
}

int string_to_int(const char *str)
{
    char *end_ptr;
    const long num = strtol(str, &end_ptr, 10);

    // invalid format was given, return -1
    if (*end_ptr != '\0' && *end_ptr != '\n') return -1;
    return (int)num;
}

voter create_voter(char* name, char* surname, const int pin, const int zipcode)
{
    const voter v = custom_malloc(sizeof(*v));
    v->name = name;
    v->surname = surname;
    v->PIN = pin;
    v->TK = zipcode;
    v->voted = 'n';  // by default not voted
    return v;
}

static bool open_file(const database db, const char* file_name)
{
    FILE* file = fopen(file_name, "r");
    if (file == NULL) return false;

    char line[LINE_SIZE];  // line buffer

    // get each line
    while (fgets(line, sizeof(line), file))
    {
        // id
        char *p = strtok(line, " ");
        if (p == NULL) continue;
        const int pin = string_to_int(p);
        if (pin == -1) continue;

        // fname
        p = strtok(NULL, " ");
        if (p == NULL) continue;
        char* name = mystrcpy(p);

        // lname
        p = strtok(NULL, " ");
        if (p == NULL)
        {
            free(name);
            continue;
        }
        char* surname = mystrcpy(p);

        // zip
        p = strtok(NULL, " ");
        if (p == NULL) 
        {
            free(name);
            free(surname);
            continue;
        }
        const int zip = string_to_int(p);
        if (zip == -1)
        {
            free(name);
            free(surname);
            continue;
        }

        // create a new paricipant and insert him in the database
        db_participant_insert(db, create_voter(name, surname, pin, zip));
        // db_participant_insert(db, create_voter(surname, name, pin, zip));
    }
    
    fclose(file);
    return true;
}

database open_cmd(int argc, char* argv[])
{
    // look for the correct commandline arguments
    char* file_name = NULL;
    int buckets = -1;
    size_t starting_size = 0;
    int expand_func = 0;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' && strlen(argv[i]) == 2)
        {
            if (i == argc-1) continue;

            if (argv[i][1] == 'f')  // -f <file_name>
                file_name = argv[i+1];
            else if (argv[i][1] == 'b')  // -b <bucket_size>
                buckets = string_to_int(argv[i+1]);
            else if (argv[i][1] == 'm')  // -m <starting_size>
                starting_size = (size_t)string_to_int(argv[i+1]);
            else if (argv[i][1] == 'e')  // -e <expand_function>
                expand_func = string_to_int(argv[i+1]);
        }
    }

    if (buckets <= 0) buckets = DEFAULT_BUCKET_SIZE;
    if (starting_size == 0) starting_size = DEFAULT_ST_CAPACITY;
    if (expand_func != 1 && expand_func != 2) expand_func = DEFAULT_EXPAND_FUNC;

    const database db = db_create(buckets, starting_size, expand_func);

    if (file_name != NULL)  // read the file, if that option was given
    {
        if (!open_file(db, file_name))
        {
            db_close(db);
            return NULL;
        }
    }

    return db;
}

// compare function used for sorting the list of zipcodes
// descending order
int comp_list(void* a, void* b)
{
    const postcode l1 = (postcode)a;
    const postcode l2 = (postcode)b;

    const int ls_1 = list_size(l1->voters);
    const int ls_2 = list_size(l2->voters);

    return ls_2 - ls_1;
}
