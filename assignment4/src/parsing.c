#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void 
parse_args(int argc, char **argv, char **dirA, char **dirB, char **dest, bool *force)
{
    if (argc < 4 || argc > 7) {
        fprintf(stderr, "Usage: %s -d dirA dirB [-s dest]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-d")) {
            if (i + 2 >= argc) {
                fprintf(stderr, "%s: missing directory name\n", argv[0]);
                exit(EXIT_FAILURE);
            }

            *dirA = argv[i+1];
            *dirB = argv[i+2];
        }
        else if (!strcmp(argv[i], "-s")) {
            if(i + 1 >= argc) {
                fprintf(stderr, "%s: missing destination name\n", argv[0]);
                exit(EXIT_FAILURE);
            }

            *dest = argv[i+1];
        }
        else if (!strcmp(argv[i], "-f")) {
            *force = true;
        }
    }

    if(*dirA == NULL) {
        fprintf(stderr, "Usage: %s -d dirA dirB [-s dest]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}
