# Makefile for the filesystem program

# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -Werror

# Target executable
TARGET = schd

# Source files
SRCS = scheduler.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Compile the source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJS) $(TARGET)