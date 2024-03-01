BIN = bin
SRC = src
MODULES = modules
INCLUDE = ./include
OBJ = $(BIN)/obj

SOURCE_FILES := $(wildcard $(SRC)/*.c)
SOURCE_FILES += $(wildcard $(SRC)/$(MODULES)/*.c)
SOURCE_FILES := $(notdir $(SOURCE_FILES))
OBJECTS = $(SOURCE_FILES:%.c=$(OBJ)/%.o)
OUT     = cmpcat
CC      = gcc
CFLAGS  = -g3 -pedantic -Wextra -Wall -I$(INCLUDE)
LFLAGS  = 
DFLAGS  = -DDEBUG

FOLDER_A = ./A
FOLDER_B = ./B
FOLDER_C = ./C

# Compare
ARGS    = -d $(FOLDER_A) $(FOLDER_B)

# Merge
ARGS    += -s $(FOLDER_C) -f

build: create_directories $(OUT)
	@echo Build complete

$(OUT):  $(OBJECTS) 
	$(CC) -o $(OUT) $(CFLAGS) $(OBJECTS) $(LFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ)/%.o: $(SRC)/$(MODULES)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

create_directories:
	@mkdir -p $(OBJ)

.PHONY: 
	clear
	build
	run
	leaks
	debug

clear: 
	rm -r -f $(BIN) $(OUT)

run:
	./$(OUT) $(ARGS)

leaks:
	valgrind --leak-check=full ./$(OUT) $(ARGS)

debug: CFLAGS += $(DFLAGS)
debug: build
