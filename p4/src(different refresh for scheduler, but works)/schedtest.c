// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        schedtest.c
// Other Files:      loop.c
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
 * schedtest.c
 * A program that creates two loop processes with given timeslices and that
 * sleep for a specified amount of time before running their loops. The program
 * then sleeps and prints out the compticks of both loop processes
 */

#include "types.h"
#include "user.h"
#include "pstat.h"
#include "param.h"

/**
 * Main function of the program. Parses arguments, creats the two loop children
 * with the correct timeslices and arguments, sleeps, prints out the compticks
 * of the children, waits for them to finish, and exit.
 *
 * argc: number of command line arguments
 * argv: array of command line arguments in string form
 */
int main(int argc, char *argv[]) {
    // if there are not 6 arguments (program name, sliceA, sleepA, sliceB,
    // sleepB, sleepParent), print an error and exit the program
    if (argc != 6) {
        printf(2, "schedtest: Invalid command line.\n");
        printf(2, "proper usage: schedtest <sliceA> <sleepA> <sliceB>");
        printf(2, " <sleepB> <sleepParent>\n");
        exit();
    }
    
    // setup for creating child processes
    int sliceA = atoi(argv[1]);  // get timeslice for loop A
    int sliceB = atoi(argv[3]);  // get timeslice for loop B
    int sleepParent = atoi(argv[5]);  // get sleep time for parent

    int pidA = fork2(sliceA);  // create loop A with given timeslice
    if (pidA == -1) {  // fork failed, print error and exit
        printf(2, "schedtest: Fork failed. sliceA must be >= 1\n.");
    } else if (pidA == 0) {  // in child
        char *argsA[3];  // argument array for loop A

        argsA[0] = "loop\0";  // first arg is executable name
        argsA[1] = argv[2];  // 2nd arg is sleep time
        argsA[2] = 0;  // 3rd arg is 0 (null)

        exec(argsA[0], argsA);  // call exec

        // if at this point, exec failed. Print error and exit
        printf(2, "schedtest: exec failed.\n");
        exit();
    }

    int pidB = fork2(sliceB);  // create loop B with given timeslice
    if (pidB == -1) {  // fork failed, print error and exit
        printf(2, "schedtest: Fork failed. sliceB must be >= 1\n.");
    } else if (pidB == 0) {  // in child
        char *argsB[3];  // argument array for loop B

        argsB[0] = "loop\0";  // first arg is executable name
        argsB[1] = argv[4];  // 2nd arg is sleep time
        argsB[2] = 0;  // 3rd arg is 0 (null)

        exec(argsB[0], argsB);  // call exec

        // if at this point, exec failed. Print error and exit
        printf(2, "schedtest: exec failed.\n");
        exit();
    }

    sleep(sleepParent);  // go to sleep

    int compticksA;  // number of compensation ticks used by A
    int compticksB;  // number of compensation ticks used by B
    int indexA = 0;  // A's index in pstat struct
    int indexB = 0;  // B;s index in pstat struct
    int both_found = 0;  // tracker if A and B's indexes are found
    struct pstat stats;  // pstat struct for getting information

    // get pstat struct filled in
    if (getpinfo(&stats) == -1) {
        // if 01 returned, passed invalid pointer, print error and exit
        printf(2, "schdtest: Invalid pointer passed to getpinfo.\n");
        exit();
    }

    // find A and B's indexes
    for (int index = 0; index < NPROC; index++) {
        if (stats.pid[index] == pidA) {
            // record A's index if found
            indexA = index;
            both_found++;  // increment tracker
        } else if (stats.pid[index] == pidB) {
            // record B's index if found
            indexB = index;
            both_found++;  // increment tracker
        }

        if (both_found == 2) {
            // both have been found, so end loop
            break;
        }
    }

    // if unable to find loop indexes, print error and exit
    if (both_found != 2) {
        printf(2, "schedtest: Unable to find loopi processes in pstat struct.\n");
        exit();
    }

    compticksA = stats.compticks[indexA];  // get A's compticks
    compticksB = stats.compticks[indexB];  // get B's compticks
    printf(1, "%d %d\n", compticksA, compticksB);

    // wait twice for children to finish
    wait();
    wait();

    exit();  // end program
}
