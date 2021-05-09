// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        loop.c
// Other Files:      schedtest.c
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
 * loop.c
 * A program with that sleeps for a certain amount of
 * time, then runs a very large workload
 */

#include "types.h"
#include "user.h"

/**
 * Main function of the program. Puts the program to sleep for a given amount
 * time, then runs a very large loop.
 *
 * argc: number of command line arguments
 * argv: array of command line arguments in string form
 */
int main (int argc, char *argv[]) {
    // check number of arguments is correct (should be 2)
    if (argc != 2) {
        // print out explanatory error message
        printf(2, "loop: Invalid command line.\n");
        printf(2, "proper usage: loop <# of sleep ticks>\n");
        exit();  // end program
    }

    int sleepT = atoi(argv[1]);  // get the number of sleep ticks
    sleep(sleepT);  // go to sleep for sleepT ticks

    int tracker = 0;  // tracks how many times loop runs
    int accumulator = 0;  // accumulates results of loop operation

    // very big loop that runs for a long time
    while (tracker < 800000000) {
        accumulator = accumulator * tracker + tracker;
        tracker++;
    }

    exit();  // end program
}
