# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
LDFLAGS =

# Save the executables here
BIN_DIR = bin

# Directories
SRC_DIR = src

# Executable program
EXEC_NAME = mysort
EXEC = $(BIN_DIR)/$(EXEC_NAME)

FILE_DIR = record_files
SIZE = 	100000

# Program command line arguments
SPLITTERS_NUM = 5
QUICKSORT_EXEC = "./bin/quick_sort"
HEAPSORT_EXEC = "./bin/heap_sort"
FILE = $(FILE_DIR)/voters$(SIZE).bin
CLA = -k $(SPLITTERS_NUM) -i $(FILE) -e1 $(QUICKSORT_EXEC) -e2 $(HEAPSORT_EXEC)

all: mysort splitter quick_sort heap_sort

# Source files
mysort: $(SRC_DIR)/coordinator.c $(SRC_DIR)/utilities.o $(SRC_DIR)/signal_handler.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(EXEC) $(SRC_DIR)/coordinator.c $(SRC_DIR)/utilities.o $(CFLAGS) $(SRC_DIR)/signal_handler.o

splitter: $(SRC_DIR)/splitter.c
	$(CC) -o $(BIN_DIR)/splitter $(SRC_DIR)/splitter.c $(SRC_DIR)/utilities.o $(CFLAGS)

quick_sort: $(SRC_DIR)/quick_sort.c
	$(CC) -o $(BIN_DIR)/quick_sort $(SRC_DIR)/quick_sort.c $(SRC_DIR)/utilities.o $(CFLAGS)

heap_sort: $(SRC_DIR)/heap_sort.c
	$(CC) -o $(BIN_DIR)/heap_sort $(SRC_DIR)/heap_sort.c $(SRC_DIR)/utilities.o $(CFLAGS)

utilities.o: $(SRC_DIR)/utilities.c
	$(CC) -c $(SRC_DIR)/utilities.c $(CFLAGS)

signal_handler.o: $(SRC_DIR)/signal_handler.c
	$(CC) -c $(SRC_DIR)/signal_handler.c $(CFLAGS)

# Phony targets
.PHONY:
	all clear help run final splitter sorter quick_sort heap_sort

# Run the program - print output to the terminal
run:
	./$(EXEC) $(CLA)

# Run the program - print output to an output file
file:
	./$(EXEC) $(CLA) > output.txt

rtest:
	./$(EXEC) $(CLA) > output.txt
	gcc -o test $(SRC_DIR)/utilities.o ./records/sort_records.c
	./test $(FILE) > test.txt
	python3 test.py

clear:
	rm -rf bin $(SRC_DIR)/utilities.o $(SRC_DIR)/signal_handler.o

# Use valgrind
help: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --trace-children=yes ./$(EXEC) $(CLA) > output.txt
