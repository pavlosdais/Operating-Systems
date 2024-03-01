# source directory
SRC_DIR = ./src

# modules directory
MOD_DIR = ./modules

EXEC = mvote
CC = gcc
flags = -Wall -Wextra -Werror -g

# object files needed
OBJ = $(SRC_DIR)/utilities.o \
	  $(SRC_DIR)/mvote.o \
	  $(SRC_DIR)/database.o \
	  $(SRC_DIR)/commands.o \
	  $(MOD_DIR)/linear_hashing.o \
	  $(MOD_DIR)/list.o \

# command line arguments
BUCKETS_NUM = 5  # The number of elements that can fit in the bucket
STARTING_SIZE = 2  # The starting number of buckets
EXPAND_FUNCT = 1  # The chosen expand function
VOTER_NUM = 500
TEST_DIR = ./test_files/voters$(VOTER_NUM).csv  # update accordinigly the path to the test files
CLA = -f $(TEST_DIR) -b $(BUCKETS_NUM) -m $(STARTING_SIZE) -e $(EXPAND_FUNCT)

# make the executable file
$(EXEC): $(OBJ)
	$(CC) -o $(EXEC) $(OBJ) $(flags)

# make the object files needed

# SRC
utilities.o: $(SRC_DIR)/utilities.c 
	$(CC) -c $(SRC_DIR)/utilities.c $(flags)

mvote.o: $(SRC_DIR)/mvote.c
	$(CC) -c $(SRC_DIR)/mvote.c $(flags)

database.o: $(SRC_DIR)/database.c
	$(CC) -c $(SRC_DIR)/database.c $(flags)

commands.o: $(SRC_DIR)/commands.c
	$(CC) -c $(SRC_DIR)/commands.c $(flags)

# MODULES
linear_hashing.o: $(MOD_DIR)/linear_hashing.c
	$(CC) -c $(MOD_DIR)/linear_hashing.c $(flags)

list.o: $(MOD_DIR)/list.c
	$(CC) -c $(MOD_DIR)/list.c $(flags)

# delete excess object files
clean:
	rm -f $(OBJ) $(EXEC)

# play the game
run: $(EXEC)
	./$(EXEC) $(CLA)

# run valgrind
help: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./$(EXEC) $(CLA)
