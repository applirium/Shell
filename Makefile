# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall

# Directories
SRCDIR = ./source
BINDIR = ./bin
LIBDIR = ./lib

# Source files
SRCS = $(wildcard $(SRCDIR)/*.c)

# Object files
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BINDIR)/%.o)

# Library file
LIB = $(LIBDIR)/help.ar

# Output executable
OUTPUT = main.exe

.PHONY: all clean

# Default target
build: $(OUTPUT)

# Rule to compile source files
$(BINDIR)/%.o: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule to create library file
$(LIB): $(BINDIR)/help.o | $(LIBDIR)
	ar rcs $@ $<

# Rule to link object files and create executable
$(OUTPUT): $(OBJS) $(LIB)
	$(CC) -o $@ $(SRCDIR)/main.c -L$(LIBDIR) $(LIB)

# Create directories if they don't exist
$(BINDIR):
	mkdir -p $(BINDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

# Clean rule
clean:
	rm -rf $(BINDIR) $(LIBDIR) $(OUTPUT)