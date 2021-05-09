// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        syscalls.c
// Other Files:      All other files in src
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
 * syscalls.c
 * A program that calls a specified number of system calls, of which some are
 * specified to fail, then calls getnumsyscalls and getnumsyscallsgood and
 * prints out the results
 */

#include "types.h"
#include "user.h"

/**
 * Function that calls a system call which completes successfully (is good)
 * a specified number of times
 *
 * num_calls: the number of times a syscall will be run successfully
 */
void GoodCalls(int num_calls) {
    // call getpid num_calls times
    for (int index = 0; index < num_calls; index++) {
        getpid();  // getpid always succeeds
    }
}

/**
 * Function that calls a system call which fails (returns -1)
 * a specified number of times
 *
 * num_calls: the number of times a syscall will be run unsuccessfully
 */
void BadCalls(int num_calls) {
    // call sleep(-1) num_calls times
    for (int index = 0; index < num_calls; index++) {
        kill(-1);  // kill(-1) always fails
    }
}

/**
 * Main function of the program. Parses input to get the number of system
 * calls to run in total and the number to run successfully, then calls the
 * functions which run system calls that run successfuly/fail
 *
 * argc: the number of command line arguments
 * argv: the array of command line arguments
 */
int main(int argc, char* argv[]) {
    // if there are not three arguments (program name, # calls, # good calls)
    // print an error and exit the program
    if (argc != 3) {
        printf(2, "syscalls: invalid command line\n");
        printf(2, "proper usage: syscalls <# total syscalls>");
        printf(2, " <# good syscalls>\n");
        exit();
    }

    int total_calls = atoi(argv[1]);  // get the total number of syscalls
    int good_calls = atoi(argv[2]);  // get the number of good syscalls

    // if the total number of syscalls is less than 1, print out an error and
    // exit
    if (total_calls < 1) {
        printf(2, "syscalls: the total number of syscalls must be >= 1\n");
        exit();
    }

    // if the number of good syscalls is less than 1, print out an error and
    // exit
    if (good_calls < 1) {
        printf(2, "syscalls: the number of good syscalls must be >= 1\n");
        exit();
    }

    // if the number of good syscalls is greater than the total number of
    // syscalls, print out an error and exit
    if (good_calls > total_calls) {
        printf(2, "syscalls: the number of total syscalls must be >= the");
        printf(2, " number of good syscalls\n");
        exit();
    }

    // get the pid of the process, which succeeds (is a good call)
    int pid = getpid();
    GoodCalls(good_calls - 1);  // make the rest of the good syscalls

    // make the remaining number of calls, which must fail
    BadCalls(total_calls-good_calls);

    // get the return value of getnumsyscalls
    int total_calls_result = getnumsyscalls(pid);

    // get the return value of getnumsyscallsgood
    int good_calls_result = getnumsyscallsgood(pid);

    // print out the return values of getnumsyscalls and getnumsyscallsgood
    printf(1, "%d %d\n", total_calls_result, good_calls_result);

    exit();  // end the program
}
