#ifndef __ALIAS__H__
#define __ALIAS__H__

#include "status.h"
#include "command.h"

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

status_t add_alias(alias_table_t *table, char *name, char *command, unsigned short overwrite);
void remove_alias(alias_table_t *table, char *entry);
alias_t *find_alias(alias_table_t *table, char *alias);
void print_aliases(alias_table_t *table);
void clear_aliases(alias_table_t *table);

#endif
