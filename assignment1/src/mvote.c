#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/utilities.h"
#include "../include/commands.h"
#include "../include/types.h"
#include "../include/database.h"

int main(int argc, char* argv[])
{
    // create database
    database db = open_cmd(argc, argv);
    if (db == NULL)
    {
        printf("Error at command line arguments.\n");
        exit(EXIT_FAILURE);
    }
    
    // create buffer
    char* buffer = custom_malloc(BUFFER_SIZE * sizeof(char));

    command_t command_n;

    while (true)
    {
        // read command from input
        fgets(buffer, BUFFER_SIZE, stdin);

        // process the command
        command_preprocess(buffer);

        if (buffer[0] == '\n') continue;
        else if (buffer[0] == '\0')
        {
            unsuccessful_response("Unknown command!");
            continue;
        }

        // tokenize
        char* p = strtok(buffer, " ");
        
        if ((command_n = command_num(p)) == FIND_PIN)  // command 1
            find_participant(db);
        
        else if (command_n == INSERT_HASH)  // command 2
            insert_participant(db);
        
        else if (command_n == VOTED)  // command 3
            mark_voted(db);
        
        else if (command_n == VOTED_FILE)  // command 4
            voters_file(db);
        
        else if (command_n == VOTER_NUM)  // command 5
            participants_num(db);
        
        else if (command_n == VOTER_PER)  // command 6
            vote_percentage(db);
        
        else if (command_n == ZIP_NUM)  // command 7
            zipcode_voters(db);
        
        else if (command_n == TK_VOTERS)  // command 8
            postcode_voters(db);
        
        else if (command_n == EXIT)  // command 9
            break;
        
        else if (command_n == PRINT)  // command 10 - my addition
            print_db(db);
        
        else  // functionality not recognized
            unsuccessful_response("unknown command");
    }

    // uncomment to exit before destroying and check with valgrind if the lost bytes are the same as the bytes released below
    // exit(EXIT_SUCCESS);

    // destroy the memory used by the buffer and close the database
    free(buffer);
    printf("%ld of Bytes Released\n", db_close(db) + BUFFER_SIZE*sizeof(char));

    exit(EXIT_SUCCESS);
}
