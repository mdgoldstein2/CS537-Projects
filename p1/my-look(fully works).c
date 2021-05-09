///////////////////////////////////////////////////////////////////////////////
// This File:        my-look.c
// Other Files:      my-rev.c
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
 * my-look.c
 * A program that functions similarly to the look utility. Prints out all words
 * in a file (or stdin) that start with a given string.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Method that prints out help information about my-look. Called when -h
 * is selected as an option.
 */
void PrintHelpStatements() {
    // Help information about my-look
    printf("my-look help information\n");
    printf("my-look: display all lines starting with a given");
    printf(" string\n");
    printf("format: ./my-look [options] <string>\n");
    printf("options:\n");
    printf("\t-V: information about my-look\n");
    printf("\t-h: help information about my-look\n");
    printf("\t-f <filename>: uses <filename> as the input");
    printf(" dictionary\n");
    printf("\t\t       if this option is not specified, stdin is used as");
    printf(" the input dictionary\n");
}

/**
 * Method that reads the dictionary file in one line at a time, and if the line
 * starts with the prefix, the method will print out that line
 *
 * dict_file: the file to be read from
 * prefix: the string that all printed lines will start with
 */
void PrintIfPrefix(FILE *dict_file,  char *prefix) {
     char buffer[128]; // buffer used to read in lines

     // read through the file one line at a time
     while (fgets(buffer, 128, dict_file) != NULL) {
	 // if the start of the line matches the prefix string,
	 // print out the line to stdout
         if (strncasecmp(buffer, prefix, strlen(prefix)) == 0) {
	     printf("%s", buffer);
	 }
     }
}

/**
 * Main method of the program. Checks the program's arguments and opens the
 * file to be searched (if specified) (add more?)
 * 
 * argc: the number of command line arguments
 * argv: array of command line arguments
 * Return: 0 to end the program
 */
int main (int argc, char* argv[]) {
    int opt; // the return value of the getopt function
    opterr = 0; // global var for getopt, suppresses getopt's built in errors
    FILE *dictionary = NULL; // the file to be read
    char *string = NULL; // the string to be used when finding lines in the file

    // Parse the arguments. -V prints out utility informatio and exits, -h
    // prints out help information and exits, and -f <filename> specifies the
    // file to use (if that option is not taken, stdin will be used)
    while ((opt = getopt(argc, argv, "Vhf:")) != -1) {
	// switch case used for parsing
	switch (opt) {
	    case 'V': // print statements for -V and exit
	        printf("my-look from CS537 Spring 2021\n");
		exit(0);
		break;
	    case 'h': // print statements for -h and exit
		PrintHelpStatements();
		exit(0);
		break;
	    case 'f': // open the file specified after -f
		dictionary = fopen(optarg, "r");

		// if the file did not open correctly, print an error and exit
    		if (dictionary == NULL) {
        	    printf("my-look: cannot open file\n");
        	    exit(1);
    		}		
		break;
	    default: // print an error and exit if any other option was used
		printf("my-look: invalid command line\n");
		exit(1);
		break;
	}
    }

    // if the -f option was not used (dictionary is NULL), use stdin as the
    // dictionary (set dictionary equal to stdin)
    if (dictionary == NULL) {
	dictionary = stdin; 
    }

    // make sure there were not too many to too few arguments (the last
    // argument is the string to be used and there are no more strings)
    if (optind + 1 != argc) {
        printf("my-look: invalid command line\n");
        exit(1);
    }

    string = argv[optind]; //get the string to be used

    // print out all lines that start with the given string
    PrintIfPrefix(dictionary, string);

    // if the dictionary is a proper file (not stdin), close it
    if (dictionary != stdin) {
        fclose(dictionary);
    }

    return 0; // return 0 to end the program
}
