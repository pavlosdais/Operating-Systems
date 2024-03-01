#ifndef PARSING_H
#define PARSING_H 

#include <stdbool.h>

// parse command line arguments
void parse_args(int argc, char **argv, char **dirA, char **dirB, char **dest, bool *force);

#endif // PARSING_H
