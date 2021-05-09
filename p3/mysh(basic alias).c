// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        mysh.c
// Other Files:      aliases.h, aliases.c
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
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "aliases.h"


// size of input buffer, with enough space to check if input > 512 chars
const int kBufferSize = 514;
const int kMaxLineLength = 512;  // maximum allowable line length
const int kNumTokens = 100;  // maximum number of tokens per command

/**
 * Function that parses the input buffer to get the arguments for the process to
 * be run, along with doing some error checking and finding any output
 * redirect files
 *
 * input: the input string to be parsed
 * output_file: the pointer to the output file to be redirected to
 * new_args: array of strings that are the arguments for the process to be run
 * Return: 0 if there are no errors, or -1 if there was an error or empty line
 */
int parse(char *input, char **output_file, char *new_args[]) {
    char *redirect_pointer;  // points to location of redirect character
    *output_file = NULL;  // set the output file to null by default

    // find the position of the redirect character
    redirect_pointer = strchr(input, '>');
 
    // if the pointer is not null, there is a redirect character
    if (redirect_pointer != NULL) {
        *redirect_pointer = '\0';  // add null terminator in place of >

        // get the output file using strtok()
        *output_file = strtok(redirect_pointer + 1, " >\t\n\r");

        // check that there is an output file
        if (*output_file == NULL) {
            // if there is not, print an error and return -1
            fprintf(stderr, "Redirection misformatted.\n");
            fflush(stderr);
            return -1;
        }

        // check that there are not multiple > characters or multiple files
        // using strtok()
        if (strtok(NULL, " >\t\n\r") != NULL) {
            // if there are multiple > characters or multiple files, print error
            // and return -1 to indicate error
            fprintf(stderr, "Redirection misformatted.\n");
            fflush(stderr);
            return -1;
        }
    }

    char *argument;  // pointer to the current argument
    int arg_index = 0;  // index of the current argument


    // use strtok() to get the arguments for the command (besides the output
    // file, if used). Finish when strtok returns null.
    while ((argument = strtok(input, " \t\n\r")) != NULL) {
        // if input is not null, make it null
        if (input != NULL) {
            input = NULL;
        }

        new_args[arg_index] = strdup(argument);  // duplicate argument to array
        arg_index++;  // increase the argument index
    }

    new_args[arg_index] = NULL;  // add NULL to end of array

    // if arg_index is still 0, the line has no arguments before redirection
    if (arg_index == 0) {
        // if there was a file to redirect to, redirection was misformatted
        if (*output_file != NULL) {
            // print an error
            fprintf(stderr, "Redirection misformatted.\n");
            fflush(stderr);
        }

        // else the line was just empty, but still return -1
        return -1;
    } else {
        return 0;  // return 0 on successful parsing
    }
}

/**
 * Function that frees the arguments from the argument array, and the
 * array itself if selected.
 *
 * arg_array_addr: address of the argument array
 * full_free: 1 if full array is to be free, any other value otherwise
 */
void FreeArgs(char*** arg_array_addr, int full_free) {
    // free argument pointers
    for (int index = 0; index < kNumTokens; index++) {
        if ((*arg_array_addr)[index] != NULL) {
            free((*arg_array_addr)[index]);
            (*arg_array_addr)[index] = NULL;  // set to null after freeing
        }
    }

    // if full free is selected, free the array pointer
    if (full_free == 1) {
        free(*arg_array_addr);
        *arg_array_addr = NULL;
    }
}

/**
 * Function that opens a file and prints out the specified error message if
 * the file was unable to be opened.
 *
 * filename: the name of the file to be opened
 */
FILE *OpenFile(char *filename) {
    FILE *open_file = fopen(filename, "r");  // open the file

    // Check that fopen() did not return NULL. If it did print out an error
    // and exit
    if (open_file == NULL) {
        // print error and flush buffer
        fprintf(stderr, "Error: Cannot open file %s.\n", filename);
        fflush(stderr);
        exit(1);  // exit
    }

    return open_file;  // else return the file pointer
}

/**
 * Function that closes a file if it was not stdin. Prints out an error if
 * there was an issue when closing.
 *
 * closing_file: the file to be closed
 */
void CloseFile(FILE *closing_file) {
    // close the file if it is not stdin
    if (closing_file != stdin) {
        if (fclose(closing_file) != 0) {
            // if there was an error, print out error and flush buffer
            fprintf(stderr, "Error: Issue closing file.\n");
            fflush(stderr);
            exit(1);  // exit
        }
    }
}

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
        fprintf(stderr, "Usage: mysh [batch-file]\n");
        fflush(stderr);
        exit(1);
    } else if (argc == 2) {
        // if there are 2 arguments, the second must be the batch file's name
        input_file = OpenFile(argv[1]);  // open the file
    } else {  // if there is no batch file, use stdin for input
        input_file = stdin;  // set input file to stdin
    }

    char *output = NULL;  // output file string

    // set up heap array of arguments for new process
    char **args = calloc(kNumTokens, sizeof(char*));

    // make sure calloc did not return null. If it did, print an error and exit
    if (args == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory.\n");
        fflush(stderr);
        exit(1);
    }

    // if in user mode, print prompt
    if (argc == 1) {
        printf("mysh> ");
        fflush(stdout);
    }

    // while loop runs until it reaches the end of the file, an exit command,
    // or the user hitting CTRL-D
    while (fgets(input_buffer, kBufferSize, input_file) != NULL) {
        // if in batch mode, echo input
        if (argc == 2) {
            printf("%s", input_buffer);
            fflush(stdout);
        }

        // check that line was not more than 512 characters
        int too_long = 0; // tracker for line being too long. 0 = not too long
        if (strlen(input_buffer) > kMaxLineLength) {
            // if line more than 512 chars, print error and set tracker
            fprintf(stderr, "Error: Command longer than 512 characters.\n");
            fflush(stderr);
            too_long = 1;

            // clear out the rest of the line if necessary (the oversized line
            // not fit into the slightly extended buffer)
            if (input_buffer[kMaxLineLength] != '\n' &&
                     input_buffer[kMaxLineLength] != EOF) {
                while(fgets(input_buffer, kBufferSize, input_file) != NULL &&
                        input_buffer[strlen(input_buffer) - 1] != '\n' &&
                                input_buffer[strlen(input_buffer) - 1] != EOF) {
                    // keep going until find end of line or file
                }
            }
        }

        // parse input. If -1 or line too long, do not run line
        if (parse(input_buffer, &output, args) != -1 && too_long != 1) {
            // check if the exit command was entered, exit loop if so
            if (strcmp(args[0], "exit") == 0) {
                break;
            } else if (strcmp(args[0], "alias") == 0) {
                AddAlias(args);
            } else if (strcmp(args[0], "unalias") == 0) {
    
            } else {
                CheckAlias(args);  // swap in alias arguments if alias exists
                int pid = fork();  // fork the process and get the pid

                if (pid == 0) {  // if pid is 0, is in child
                    if (output != NULL) {  // check if output is redirected
                        // open the file descriptor for the new output file
                        int new_out_desc = open(output,
                                O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

                        // if the new fd is -1, unable to open file, so
                        // print error and exit
                        if (new_out_desc == -1) {
                            // print error
                            fprintf(stderr, "Cannot write to file %s.\n",
                                    output);
                            fflush(stderr);

                            // close out/free array and file
                            FreeArgs(&args, 1);  // free arg array
                            CloseFile(input_file);  // close the input file
                            _exit(1);  // exit without cleanup
                        }

                        // close out stdout, have its file descriptor be a
                        // copy of the new descriptor. If it returned -1,
                        // there was an error, so print error and exit.
                        if (dup2(new_out_desc, STDOUT_FILENO) == -1) {
                            // print out the error
                            fprintf(stderr, "Error: dup2 failed.\n");
                            fflush(stderr);

                            // close out/free array and file
                            FreeArgs(&args, 1);  // free arg array
                            CloseFile(input_file);  // close the input file
                            _exit(1);  // exit without cleanup
                        }
                    }

                    execv(args[0], args);  // execute the new processs

                    // if reached this point, execv has failed, print out error
                    fprintf(stderr, "%s: Command not found.\n", args[0]);
                    fflush(stderr);

                    // close out/free array and file
                    FreeArgs(&args, 1);  // free arg array
                    CloseFile(input_file);  // close the input file
                    _exit(1);  // exit without cleanup
                } else if (pid == -1) {  // if pid is -1, fork failed
                    fprintf(stderr, "Error: Fork failed.\n");  // print error
                    fflush(stderr);
                } else {  // else still in parent
                    int wait_status;  // wait status indicator
                    waitpid(pid, &wait_status, 0);  // wait until child finishes
                }
            }
        }

        // if in user mode, print out prompt
        if (argc == 1) {
            printf("mysh> ");
            fflush(stdout);
        }

        // free only argument pointers once process ended
        FreeArgs(&args, 0);
    }

    // close the batch file at the end of the program
    CloseFile(input_file);

    // free argument pointers and array after exit
    FreeArgs(&args, 1);

    return 0;  // end program
}
