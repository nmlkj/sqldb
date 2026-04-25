# Compiler settings
CC = gcc
CFLAGS = -Wall -I../sql_compiler -I../sql_vm -I../storage_engine/csv_engine -g

# Directories
COMPILER_DIR = ../sql_compiler
VM_DIR = ../sql_vm
STORAGE_DIR = ../storage_engine/csv_engine

# Target executable
TARGET = sql_shell

# Source files
# We assume the shell main file will be main.c within the sql_shell directory
SRCS = main.c \
       $(COMPILER_DIR)/compiler.c \
       $(COMPILER_DIR)/parser.tab.c \
       $(COMPILER_DIR)/lex.yy.c \
       $(VM_DIR)/vm.c \
       $(STORAGE_DIR)/storage_engine.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Debug build with full symbol information and debug flags
debug: CFLAGS += -g3 -O0 -DDEBUG
debug: clean $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lfl

# Rules to build individual object files from different directories
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Special rule for Flex/Bison generated files if they need regenerating
# Note: Requires flex and bison to be installed
parser:
	bison -d $(COMPILER_DIR)/parser.y -o $(COMPILER_DIR)/parser.tab.c
	flex -o $(COMPILER_DIR)/lex.yy.c $(COMPILER_DIR)/scanner.l

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(OBJS)
	rm -f $(COMPILER_DIR)/*.o $(VM_DIR)/*.o $(STORAGE_DIR)/*.o

.PHONY: all clean parser