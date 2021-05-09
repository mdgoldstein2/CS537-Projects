// Copyright 2021 Michael Goldstein
///////////////////////////////////////////////////////////////////////////////
// This File:        aliases.c
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

// struct that defines the linked list (just has head)
typedef struct AliasList {
    struct AliasListEntry *head; // pointer to head of list
} AliasList;

// function declarations
void Unalias(char*[]);  // for alias command
void AddAlias(char*[]);  // for unalias command
void CheckAlias(char*[]);  // for checking if command has alias
void CloseAliases();  // for clearing alias memories
