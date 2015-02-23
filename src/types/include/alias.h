#ifndef __ALIAS__H__
#define __ALIAS__H__

#include "command.h"
#include "status.h"

#define ALIAS_BUCKETS 128

/**
  * Holds information about an alias, including the name of the alias and the command associated
  * with that name. Also includes a next pointer for use in a linked-list/hash table
  */
typedef struct alias_t
{
	char *alias;
	command_t *command;
	struct alias_t *next;
} alias_t;

/**
  * A hash table for the aliases.
  */
typedef struct
{
	alias_t *alias_entries[ALIAS_BUCKETS];
} alias_table_t;


/**
  * Adds an alias entry to the given table, with alias name name and command indicated by the quoted
  * string command
  * @param table     the table into which the new entry should be placed
  * @param name      the name of the new alias
  * @param command   the (quoted) command string (e.g., the string, including quotes, "ls -al")
  * @param overwrite if the alias already exists in the table, determiens if it will be overwritten
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t add_alias(alias_table_t *table, char *name, char *command, unsigned short overwrite);

/** 
  * Removes an alias entry from the given table with alias name entry
  * @param table the table from which the entry should be removed
  * @param entry the alias name of the entry to be removed
  */
void remove_alias(alias_table_t *table, char *entry);

/** 
  * Finds the alias in the table with the given alias name
  * @param table the table to be searched for the alias
  * @param alias the alias name of the entry being looked for
  * @return the alias_t entry in the table with given alias or NULL if alias not in the table
  */
alias_t *find_alias(alias_table_t *table, char *alias);

/**
  * Prints the aliases in the table
  * @param table the table to be printed
  */
void print_aliases(alias_table_t *table);

/**
  * Clears and frees the memory associated with the alias table
  * @param table the table to be cleared
  */
void clear_aliases(alias_table_t *table);

#endif
