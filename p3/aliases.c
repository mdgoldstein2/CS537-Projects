// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        aliases.c
// Other Files:      aliases.h, mysh.c
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
 * aliases.c
 * Contains methods used to maintain an alias list (as defined in aliases.h).
 */

#include "aliases.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

AliasList aliases;  // list of aliases
extern const int kNumTokens;  // maximum number of tokens per command

/**
 * Function that allocates space for a new alias list entry and provides
 * error checking for memory allocation.
 *
 * Return: pointer to memory with space for an alias list entry
 */
AliasListEntry *AllocateSpace() {
    // allocate space for new alias entry
    AliasListEntry *new_alias = calloc(1, sizeof(AliasListEntry));

    // make sure calloc did not fail. If it did, print error and exit
    if (new_alias == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory.\n");
        fflush(stderr);
        exit(1);
    }

    return new_alias;  // return pointer
}

/**
 * Function that allocates space for an argument array and provides error
 * checking for memory allocation.
 * 
 * Return: pointer to memory with space for an argument array
 */
char **AllocateArgs() {
    // create array of arguments
    char **arg_array = calloc(kNumTokens, sizeof(char*));

    // make sure calloc did not fail. If it did, print error and exit
    if (arg_array == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory.\n");
        fflush(stderr);
        exit(1);
    }

    return arg_array;  // return pointer
}

/**
 * Function that prints out an alias and its arguments.
 *
 * args: array of arguments, also containing alias
 */
void PrintArgs(char *args[]) {
    int arg_tracker = 0;  // tracks arguments

    // loop through and print out args
    while (args[arg_tracker] != NULL) {
        printf("%s", args[arg_tracker]);  // print arg
        fflush(stdout);

        // if not the last arg, print a space
        if (args[arg_tracker + 1] != NULL) {
            printf(" ");
            fflush(stdout);
        }

        arg_tracker++;  // increment tracker
    }

    // print newline
    printf("\n");
    fflush(stdout);
}

/**
 * Function that prints out all alias entries and their corresponding tokens
 */
void PrintAliases() {
    // iterate through alias list starting from head
    AliasListEntry *curr = aliases.head;  // get head

    if (curr == NULL) {  // head has not been allocated (list empty)
        return;  // return early
    } else {  // head allocated so print out information
        while (curr != NULL) {  // print out tokens for each alias
            PrintArgs(curr->alias_args);
            curr = curr->next;  // move to next node
        }
    }
}

/**
 * Function that frees an argument array.
 *
 * args: the argument array to be freed.
 */
void FreeAliasArgs(char *args[]) {
    // make sure args is not null
    if (args == NULL) {
        return;  // return if it is
    }

    // free each argument if it is not null, then mark it null
    for (int index = 0; index < kNumTokens; index++) {
        if (args[index] != NULL) {
            free(args[index]);
            args[index] = NULL;
        }
    }

    free(args);  // free the array itself
}

/**
 * Function that frees all of the memory of one AliasListEntry.
 *
 * alias: AliasListEntry to be freed
 */
void FreeAlias(AliasListEntry *alias) {
    if (alias == NULL) {  // make sure alias not NULL
        return;  // return if it is
    }

    FreeAliasArgs(alias->alias_args);  // free arguments
    alias->alias_args = NULL;  // set arguments array to null
    free(alias);  // free the alias itself
}

/**
 * Function that swaps two arrays of arguments
 *
 * args0: the array of arguments to be swapped out
 * args1: the array of arguments to be swapped in
 */
void SwapArgs(char *args0[], char *args1[]) {
    // free and replace all of args0's arguments with argd1's arguments
    int arg_tracker = 0;  // tracks arguments
    while (args1[arg_tracker] != NULL) {
        // if arg in arg0 is not null, free it
        if (args0[arg_tracker] != NULL) {
            free(args0[arg_tracker]);
        }

        // copy over corresponding arg from arg1 and increment tracker
        args0[arg_tracker] = strdup(args1[arg_tracker]);
        arg_tracker++;
    }

    // free any remaining args in arg0 that were not altered (if args0 had
    // more arguments than arg1
    for (int index = arg_tracker; index < kNumTokens; index++) {
        // if arg in arg0 is not null, free it
        if (args0[index] != NULL) {
            free(args0[index]);
            args0[index] = NULL;
        }
    }
}

/**
 * Function that removes an alias entry from the list.
 *
 * args: argument array with unalias command and alias to remove
 */
void Unalias(char *args[]) {
    // check number of arguments. If incorrect, print error and return
    if (args[1] == NULL || args[2] != NULL) {
        fprintf(stderr, "unalias: Incorrect number of arguments.\n");
        fflush(stderr);
        return;
    }

    // iterate through alias list starting from head
    AliasListEntry *curr = aliases.head;  // get head

    if (curr == NULL) {  // head has not been allocated (list empty)
        return;  // return early
    } else {  // head allocated so print out information
        while (curr != NULL) {  // try to find matching alias
            // check if the alias has already been allocated
            if (strcmp(curr->alias_args[0], args[1]) == 0) {
                // if alias has been allocated, remove it from alias list
                // set prev's next to next (if prev exists)
                if (curr->prev != NULL) {
                    curr->prev->next = curr->next;
                } else {  // else curr is head
                    aliases.head = curr->next;  // make next head
                }

                // then if next exists, set next's prev to curr's prev
                if (curr->next != NULL) {
                    curr->next->prev = curr->prev;
                }

                FreeAlias(curr);  // free the node
                return;  // do not continue looking
            }

            curr = curr->next;  // move to next node
        }
    }
}

/**
 * Function that adds an alias entry to the list.
 *
 * args: argument array with alias command, alias to add, and its corresponding
 *       tokens
 */
void AddAlias(char *args[]) {
    // check if alias command has no other args and therefore should print
    // out all aliases and their arguments
    if (args[1] == NULL) {
        PrintAliases();
        return;
    }

    // check to make sure alias is not too dangerous (the alias is not alias,
    // unalias, or exit)
    if (strcmp(args[1], "alias") == 0 ||
            strcmp(args[1], "unalias") == 0 ||
                    strcmp(args[1], "exit") == 0) {
        // if dangerous, print error and exit
        fprintf(stderr, "alias: Too dangerous to alias that.\n");
        fflush(stderr);
        return;
    }

    // iterate through alias list starting from head
    AliasListEntry *curr = aliases.head;  // get head

    // if head not allocated and alias command has tokens after the alias
    if (curr == NULL && args[2] != NULL) {  // allocate head
        aliases.head = AllocateSpace();
        curr = aliases.head;  // get head again
    } else if (curr == NULL) {
        return;  // end early if list empty and no tokens after alias in args
    } else {  // head allocated so go until find empty next node
        while (curr->next != NULL) {
            // check that the alias has not already been allocated
            if (strcmp(curr->alias_args[0], args[1]) == 0) {
                // if only alias entered, print out alias
                if (args[2] == NULL) {
                    PrintArgs(curr->alias_args);
                } else {  // else swap args
                    SwapArgs(curr->alias_args, args + 1);
                }

                return;  // do not allocate new alias then
            }

            curr = curr->next;  // move to next node
        }

        // check that the alias has not already been allocated at final entry
        if (strcmp(curr->alias_args[0], args[1]) == 0) {
            // if only alias entered, print out alias
            if (args[2] == NULL) {
                PrintArgs(curr->alias_args);
            } else {  // else swap args
                SwapArgs(curr->alias_args, args + 1);
            }

            return;  // do not allocate new alias then
        }

        // check that there are tokens after the alias to add
        if (args[2] == NULL) {
            return;  // return early if so
        }

        curr->next = AllocateSpace();  // allocate space for new node
        curr->next->prev = curr;  // set curr node as prev of new node
        curr = curr->next;  // make new node the curr node
    }

    curr->alias_args = AllocateArgs();  // allocate space for arguments

    int arg_tracker = 1;  // tracks arguments when copying to argument array
    while (args[arg_tracker] != NULL) {
        // swap over arguments with offset of 1 to not copy over alias command
        curr->alias_args[arg_tracker - 1] = strdup(args[arg_tracker]);
        arg_tracker++;  // increment tracker
    }

    curr->alias_args[arg_tracker - 1] = NULL;  // set arg after last to null
}

/**
 * Function that checks if an argument array contains an alias, and if it does,
 * swaps in the tokens corresponding to the alias.
 *
 * args: array of arguments to check for alias and swap if necessary
 */
void CheckAlias(char *args[]) {
    // iterate through alias list starting from head
    AliasListEntry *curr = aliases.head;  // get head

    if (curr == NULL) {  // head has not been allocated (list empty)
        return;  // return early
    } else {  // head allocated so list exits
        while (curr != NULL) {  // try to find matching alias
            // check if the alias has already been allocated
            if (strcmp(curr->alias_args[0], args[0]) == 0) {
                // if alias has been allocated, swap in args from alias list
                SwapArgs(args, curr->alias_args + 1);
                return;  // do not continue looking
            }

            curr = curr->next;  // move to next node
        }
    }
}

/**
 * Function that frees all memory in the alias list.
 */
void CloseAliases() {
    // iterate through alias list starting from head
    AliasListEntry *curr = aliases.head;  // get head

    if (curr == NULL) {  // head has not been allocated (list empty)
        return;  // return early
    } else {  // head allocated so list exists
        while (curr != NULL) {  // free all alias entries
            curr->prev = NULL;  // de-link previously freed alias entry
            AliasListEntry *temp = curr->next;  // get next node
            FreeAlias(curr);  // free the node
            curr = temp;  // move to next node
        }

        aliases.head = NULL;  // set head to null
    }
}
