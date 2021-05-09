// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        my-rev.c
// Other Files:      my-look.c
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
 * my-rev.c
 * A program that functions similarly to the rev utility. Reverses and prints
 * out all lines from a file (or stdin).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Method that prints out help information about my-rev. Called when -h
 * is selected as an option.
 */
void PrintHelpStatements() {
    // Help information about my-look
    printf("my-rev help information\n");
    printf("my-rev: reverse each line of a file on a character by character");
    printf(" basis\n");
    printf("format: ./my-rev [options]\n");
    printf("options:\n");
    printf("\t-V: information about my-rev\n");
    printf("\t-h: help information about my-rev\n");
    printf("\t-f <filename>: uses <filename> as the input");
    printf(" dictionary\n");
    printf("\t\t       if this option is not specified, stdin is used as");
    printf(" the input dictionary\n");
}

/**
 * Method that reads the dictionary file in one line at a time and prints each
 * line out in reverse one at a time.
 *
 * dict_file: the file to be read from
 */
void PrintInReverse(FILE *dict_file) {
    char buffer[128];  // buffer used to read in lines
    char reverse_line[128];  // buffer for reversing the line

    // read through the file one line at a time
    while (fgets(buffer, 128, dict_file) != NULL) {
        int line_length = strlen(buffer);  // get the length of the current line
        // check if the line ended in a newline character. If it did, reverse
        // the line except for the last character. Else reverse the whole line
        if (buffer[line_length - 1] == '\n') {
            // the line ends in a new line, so reverse it except
            // for the new line
            for (int index = 0; index < line_length - 1; index++) {
                reverse_line[line_length - index - 2] = buffer[index];
            }

            // add a new line character at the end of the reversed line
            reverse_line[line_length - 1] = '\n';
        } else {
            // the line does not end in a new line, so fully reverse it
            for (int index = 0; index < line_length; index++) {
                reverse_line[line_length - index - 1] = buffer[index];
            }
        }

        // add in string terminating character
        reverse_line[line_length] = '\0';

        // print out the reversed line to stdout
        printf("%s", reverse_line);
        memset(reverse_line, 0, line_length);  // clear reversing buffer
    }
}

/**
 * Main method of the program. Checks the program's arguments, prints/calls
 * methods for any special output, opens the file to be reversed (if specified),
 * and calls method to print out the lines (in reverse character by character)
 * from the file
 * 
 * argc: the number of command line arguments
 * argv: array of command line arguments
 * Return: 0 to end the program
 */
int main(int argc, char* argv[]) {
    int opt;  // the return value of the getopt function
    opterr = 0;  // global var for getopt, suppresses getopt's built in errors
    FILE *dictionary = NULL;  // the file to be read

    // Parse the arguments. -V prints out utility informatio and exits, -h
    // prints out help information and exits, and -f <filename> specifies the
    // file to use (if that option is not taken, stdin will be used)
    while ((opt = getopt(argc, argv, "Vhf:")) != -1) {
        // switch case used for parsing
        switch (opt) {
            case 'V':  // print statements for -V and exit
                printf("my-rev from CS537 Spring 2021\n");
                exit(0);
                break;
            case 'h':  // print statements for -h and exit
                PrintHelpStatements();
                exit(0);
                break;
            case 'f':  // open the file specified after -f
                dictionary = fopen(optarg, "r");

                // if the file did not open correctly, print an error and exit
                if (dictionary == NULL) {
                    printf("my-rev: cannot open file\n");
                    exit(1);
                }
                break;
            default:  // print an error and exit if any other option was used
                printf("my-rev: invalid command line\n");
                exit(1);
                break;
        }
    }

    // if the -f option was not used (dictionary is NULL), use stdin as the
    // dictionary (set dictionary equal to stdin)
    if (dictionary == NULL) {
        dictionary = stdin;
    }

    // make sure there were not too many to too few arguments (there are no
    // non-option arguments)
    if (optind != argc) {
        printf("my-rev: invalid command line\n");
        exit(1);
    }

    // print out all lines that start with the given string
    PrintInReverse(dictionary);

    // if the dictionary is a proper file (not stdin), close it
    if (dictionary != stdin) {
        fclose(dictionary);
    }

    return 0;  // return 0 to end the program
}
