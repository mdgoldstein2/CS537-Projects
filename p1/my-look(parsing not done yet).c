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
    FILE *fp = NULL; // the file to be read

    // Parse the arguments. -V prints out utility informatio and exits, -h
    // prints out help information and exits, and -f <filename> specifies the
    // file to use (if that option is not taken, stdin will be used)
    while ((opt = getopt(argc, argv, "Vhf:")) != -1) {
	// switch case used for parsing
	switch (opt) {
	    case 'V':
	        printf("my-look from CS537 Spring 2021\n");
		exit(0);
		break;
	    case 'h':
		printf("my-look help:\n");
		printf("Correct command format options
    }
    return 0; // return 0 to end the program
}
