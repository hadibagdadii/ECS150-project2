# Target library - EXAMPLE
#lib := libuthread.a
#all: $(lib)

# Makefile for libuthread
# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -std=c11

# Source files
SRCS = uthread.c private.c queue.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HDRS = uthread.h private.h queue.h

# Target library
LIBRARY = libuthread.a

.PHONY: all clean

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	ar rcs $@ $(OBJS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBRARY) $(OBJS)
