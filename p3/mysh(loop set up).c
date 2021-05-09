// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        mysh.c
// Other Files:      None
// Semester:         CS 537 Spring 2021
// Instructor:       Andrea Arpaci-Dusseau
//
// Discussion Group: Disc. 313
// Author:           Michael Goldstein
// Email:            mdgoldstein2@wisc.edu
// CS Login:         mgoldstein
//
//////////////////////////// 80 columns wide ///////////////////////////////////

/**
 * mysh.c
 * A shell program which executes commands using fork() and execv(). The shell
 * supports interactive (user types commands) and batch (file with list of
 * commands) modes. The shell also has the built in alias, unalias, and exit
 * commands.
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const int kBufferSize = 512;  // size of input/output buffers

/**
 * The main method of the shell. (add more when needed)
 *
 * argc: the number of command line arguments
 * argv: array of command line arguments
 * Return: 0 to end the program
 */
int main(int argc, char* argv[]) {
    FILE *input_file;  // input file/stdin
    char input_buffer[kBufferSize];  // input buffer

    // check that the number of arguments is correct. If there are more than 2,
    // print an error and terminate
    if (argc > 2) {
        write(STDERR_FILENO, "Usage: mysh [batch-file]\n",
                strlen("Usage: mysh [batch-file]\n"));
        exit(1);
    } else if (argc == 2) {
        // if there are 2 arguments, the second must be the batch file's name
        input_file = fopen(argv[1], "r");  // open the file

        // Check that fopen() did not return NULL. If it did print out an error
        // and exit
        if (input_file == NULL) {
            char error_buffer[kBufferSize];  // buffer for error message
            
            // compose the error message "Error: Cannot open file 
            // <filename>.\n"
            strcpy(error_buffer, "Error: Cannot open file ");
            strcat(error_buffer, argv[1]);
            strcat(error_buffer,".\n");
            
            // print out the error
            write(STDERR_FILENO, error_buffer,  strlen(error_buffer));
            exit(1);  // exit
        }
    } else {  // if there is no batch file, use stdin for input
        input_file = stdin;  // set input file to stdin
    }

    // while loop runs until it reaches the end of the file, an exit command,
    // or the user hitting CTRL-D
    while (fgets(input_buffer, input_file, kBufferSize) != NULL) {
        
    }

    // close the batch file at the end of the program
    if (input_file != stdin) {
        fclose(input_file);
    }

    return 0;  // ends program
}
