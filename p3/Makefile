# Build by running 'make mysh' or simply 'make'
# Clean by running 'make clean'

# A couple standard variables:
# C Compiler
CC=gcc
# C Flags
CFLAGS=-Wall -Werror -g

# Main build command
mysh: mysh.c aliases.h aliases.c
	$(CC) $(CFLAGS) -o mysh mysh.c aliases.c

# .PHONY means this command does not actually produce an output file
.PHONY: clean
clean:
	rm -rf mysh
