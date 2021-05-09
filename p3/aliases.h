// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        aliases.h
// Other Files:      mysh.c, aliases.c
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
 * aliases.h
 * This header provides a doubly linked list structure used for storing aliases
 */

// struct that defines a node in the linked list
typedef struct AliasListEntry {
    struct AliasListEntry *prev;  // pointer to next node
    struct AliasListEntry *next;  // pointer to prev node
    char **alias_args;  // array of alias arguments
} AliasListEntry;

// struct that defines the alias list (just has head)
typedef struct AliasList {
    struct AliasListEntry *head;  // pointer to head of list
} AliasList;

/**
 * Function that removes an alias entry from the list (implements unalias
 * command).
 *
 * char*[]: argument array with unalias command and alias to remove
 */
void Unalias(char*[]);

/**
 * Function that adds an alias entry to the list (implements alias command)
 *
 * char*[]: argument array with alias command, alias to add, and its corresponding
 *          tokens
 */
void AddAlias(char*[]);

/**
 * Function that checks if an argument array contains an alias, and if it does,
 * swaps in the tokens corresponding to the alias.
 *
 * char*[]: array of arguments to check for alias and swap if necessary
 */
void CheckAlias(char*[]);  // for checking if command has alias

/**
 * Function that frees all memory in the alias list.
 */
void CloseAliases();
