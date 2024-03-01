#include <signal.h>

// declaration of external variables to keep track of signals for sorters and splitters
extern volatile sig_atomic_t signals_arrived_splitters;
extern volatile sig_atomic_t signals_arrived_sorters;

// increments the counter when a signal is received from splitters
void signal_handler_splitters()  { signals_arrived_splitters++; }

// increments the counter when a signal is received from sorters
void signal_handler_sorters()    { signals_arrived_sorters++; }
