# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -pthread -lrt
LDFLAGS = -pthread -lrt

# Save the executables here
BIN_DIR = bin

# Directories
SRC_DIR = src
READER_DIR = $(SRC_DIR)/Reader
WRITER_DIR = $(SRC_DIR)/Writer

# Executable program
EXEC_NAME = creator
EXEC = $(BIN_DIR)/$(EXEC_NAME)
WRITE_EXEC = $(BIN_DIR)/write
READER_EXEC = $(BIN_DIR)/reader
FILE_NUM = 50
INPUT_FILE = ./input_files/accounts$(FILE_NUM).bin
LOG_FILE = logging_info.log

# Program command line arguments
WRITERS_NUM = 15
READERS_NUM = 15
WRITERS_T = 2
READERS_T = 2
LOG_LEVEL = 1
SHARED_MEM = "/shmem"
CLA = -f $(INPUT_FILE) -rn $(READERS_NUM) -wn $(WRITERS_NUM) -rt $(READERS_T) -wt $(WRITERS_T) \
	  -wp $(WRITE_EXEC) -rp $(READER_EXEC) -sm $(SHARED_MEM) -lg $(LOG_LEVEL)

all: creator reader write

# Source files
creator: $(SRC_DIR)/creator.c $(SRC_DIR)/utilities.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(EXEC) $(SRC_DIR)/creator.c $(SRC_DIR)/utilities.o $(CFLAGS)

reader: $(READER_DIR)/reader.c $(READER_DIR)/reader_utilities.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/reader $(READER_DIR)/reader.c $(SRC_DIR)/utilities.o $(READER_DIR)/reader_utilities.o $(CFLAGS)

write: $(WRITER_DIR)/write.c $(WRITER_DIR)/write_utilities.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/write $(WRITER_DIR)/write.c $(SRC_DIR)/utilities.o $(WRITER_DIR)/write_utilities.o $(CFLAGS)

utilities.o: $(SRC_DIR)/utilities.c
	$(CC) -c $(SRC_DIR)/utilities.c $(CFLAGS)

reader_utilities.o: $(READER_DIR)/reader_utilities.c
	$(CC) -c $(READER_DIR)/reader_utilities.c $(CFLAGS)

write_utilities.o: $(WRITER_DIR)/write_utilities.c
	$(CC) -c $(READER_DIR)/write_utilities.c $(CFLAGS)

# Phony targets
.PHONY:
	all clear help run final reader write creator

# Run the program - print output to the terminal
run:
	./$(EXEC) $(CLA)

# Run the program - print output to the output.txt file
file:
	./$(EXEC) $(CLA) > output.txt

clear:
	rm -rf bin $(SRC_DIR)/utilities.o $(READER_DIR)/reader_utilities.o $(WRITER_DIR)/write_utilities.o 

clear_all:
	rm -f $(LOG_FILE)
	rm -f /dev/shm$(SHARED_MEM)
	rm -rf bin $(SRC_DIR)/utilities.o $(READER_DIR)/reader_utilities.o $(WRITER_DIR)/write_utilities.o 

mod:
	chmod +w $(INPUT_FILE)

# Use valgrind
help: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --trace-children=yes ./$(EXEC) $(CLA)
