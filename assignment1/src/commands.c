#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/commands.h"
#include "../include/utilities.h"
#include "../include/list.h"
#include "../include/database.h"

// 1 - l <pin>
void find_participant(const database db)
{
    char* p = strtok(NULL, " ");
    const int pin = string_to_int(p);
    if (pin == -1)
    {
        unsuccessful_response("Malformed Pin");
        return;
    }
    
    const voter v = hash_search(db->ht, pin); 
    if (v != NULL)  // found
        printf("%d %s %s %d %c\n\n", v->PIN, v->surname, v->name, v->TK, v->voted);
    else
        fprintf(stderr, "Participant %d not in cohort\n\n", pin);
}

// 2 - i <pin> <lname> <fname> <zip>
void insert_participant(const database db)
{
    // pin
    char* p = strtok(NULL, " ");
    if (check_malformed(p)) return;

    const int pin = string_to_int(p);
    if (pin == -1)
    {
        unsuccessful_response("Malformed Input");
        return;
    }

    // lname
    p = strtok(NULL, " ");
    if (check_malformed(p)) return;
    char* surname = mystrcpy(p);

    // fname
    p = strtok(NULL, " ");
    if (check_malformed(p)) return;

    char* name = mystrcpy(p);

    // zip
    p = strtok(NULL, " ");
    if (p == NULL)
    {
        free(surname);
        free(name);
        unsuccessful_response("Malformed Input");
        return;
    }
    const int zip = string_to_int(p);
    if (zip == -1)
    {
        unsuccessful_response("Malformed Input");
        return;
    }

    if (db_participant_insert(db, create_voter(name, surname, pin, zip)))
        printf("Inserted %d %s %s %d %c\n\n", pin, surname, name, zip, 'N');
    else
        fprintf(stderr, "%d already exist\n\n", pin);
}

// 3 - m <pin>
void mark_voted(const database db)
{
    char* p = strtok(NULL, " ");
    const int pin = string_to_int(p);
    if (pin == -1)
    {
        unsuccessful_response("Malformed Input");
        return;
    }
    
    // find the voter
    const voter v = hash_search(db->ht, pin);
    if (v != NULL)  // voter found
    {
        if (v->voted == 'y')
            unsuccessful_response("Participant already voted");
        else
        {
            db_insert_voter(db, v);
            printf("%d Mark Voted\n\n", pin);
        }
    }
    else  // participant does not exist in the database
        fprintf(stderr, "%d does not exist\n\n", pin);
}

// 4 - bv <file>
void voters_file(const database db)
{
    // open the file
    char* d = strtok(NULL, " ");
    char* file_name = mystrcpy(d);

    FILE* file = fopen(file_name, "r");
    free(file_name);
    if (file == NULL) 
    {
        printf("%s could not be opened\n\n", file_name);
        return;
    }

    char line[LINE_SIZE];  // line buffer

    // get each line
    while (fgets(line, sizeof(line), file))
    {
        char *p = strtok(line, " ");
        if (check_malformed(p)) return;

        const int pin = string_to_int(p);
        if (pin == -1)
        {
            unsuccessful_response("Malformed Input");
            continue;
        }
        
        if(!db_mark_voted(db, pin))
            printf("%d does not exist\n", pin);
        else  // even if voter has already voted come here
            printf("%d Marked Voted\n", pin);
    }
    
    fclose(file);
    successful_response("");
}

// 5 - v
void participants_num(const database db)
{
    printf("Voted So Far %ld\n\n", get_voters_size(db));
}

// 6 - perc
void vote_percentage(const database db)
{
    // get number of participants
    const size_t participants = get_participants_size(db);
    
    // print with a precision of 3
    printf("%.3f\n\n", (participants == 0)? 0 : (float)get_voters_size(db) / participants * 100);
}

// 7 - z <zipcode>
void zipcode_voters(const database db)
{
    char* p = strtok(NULL, " ");
    if (check_malformed(p)) return;

    const int zipcode = string_to_int(p);
    if (zipcode == -1)
    {
        unsuccessful_response("Malformed Input");
        return;
    }

    print_zipcodes(db->list, zipcode);
}

// 8 - o
void postcode_voters(const database db)
{
    db_sort(db);
    sorted_print(db->list);
}

// 10 - p
void print_db(const database db)
{
    hash_print(db->ht);
    printf("\n");
}
