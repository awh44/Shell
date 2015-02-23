#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/alias.h"
#include "../../misc/include/parse.h"

size_t hash(char *str);
size_t alias_hash(char *name);

status_t add_alias(alias_table_t *table, char *name, char *command, unsigned short overwrite)
{
	alias_t *entry = find_alias(table, name);
	if (entry != NULL)
	{
		if (!overwrite)
		{
			return EXISTS_ERROR;
		}

		remove_alias(table, name);
	}

	//remove the quote from the command
	if (command[0] != '"')
	{
		return FORMAT_ERROR;
	}
	command++;
	size_t chars_read = strlen(command);

	if (command[chars_read - 1] != '"')
	{
		return FORMAT_ERROR;
	}

	command_t new_command;
	status_t error = parse_line(command, chars_read, &new_command);
	if (error != SUCCESS)
	{
		return error;
	}

	command_t *command_copy = malloc(sizeof *command_copy);
	if (command_copy == NULL)
	{
		free(new_command.arguments);
		return MEMORY_ERROR;
	}

	error = copy_command(command_copy, &new_command);
	free(new_command.arguments);
	if (error != SUCCESS)
	{
		free(command_copy);
		return error;
	}

	alias_t *new_alias = malloc(sizeof *new_alias);
	if (new_alias == NULL)
	{
		free(command_copy);
		return MEMORY_ERROR;
	}
	new_alias->command = command_copy;
	new_alias->alias = strdup(name);
	if (new_alias->alias == NULL)
	{
		free(command_copy);
		free(new_alias);
		return MEMORY_ERROR;
	}

	size_t hash_val = alias_hash(name);
	new_alias->next = table->alias_entries[hash_val];
	table->alias_entries[hash_val] = new_alias;

	return SUCCESS;
}

void remove_alias(alias_table_t *table, char *name)
{
	size_t hash_val = alias_hash(name);
	alias_t *entry = table->alias_entries[hash_val];

	if (entry == NULL)
	{
		return;
	}

	//handle case where alias is head of the linked list
	if (strcmp(entry->alias, name) == 0)
	{
		alias_t *to_be_freed = entry;
		table->alias_entries[hash_val] = to_be_freed->next;
		free_command(to_be_freed->command);
		free(to_be_freed->command);
		free(to_be_freed->alias);
		free(to_be_freed);
		return;
	}

	//handle case where alias is deeper in list
	while (entry != NULL)
	{
		if (entry->next && strcmp(entry->next->alias, name) == 0)
		{
			alias_t *to_be_freed = entry->next;
			entry->next = to_be_freed->next;
			free_command(to_be_freed->command);
			free(to_be_freed->command);
			free(to_be_freed->alias);
			free(to_be_freed);
			return;
		}
		entry = entry->next;
	}	
}

alias_t *find_alias(alias_table_t *table, char *alias)
{
	size_t hash_val = alias_hash(alias);
	alias_t *entry = table->alias_entries[hash_val];
	while (entry != NULL)
	{
		if (strcmp(entry->alias, alias) == 0)
		{
			return entry;
		}

		entry = entry->next;
	}

	return NULL;
}

void print_aliases(alias_table_t *table)
{
	size_t i;
	for (i = 0; i < ALIAS_BUCKETS; i++)
	{
		alias_t *entry = table->alias_entries[i];
		while (entry != NULL)
		{
			fprintf(stdout, "%s: ", entry->alias);
			print_command(entry->command);
			fprintf(stdout, "\n");
			entry = entry->next;
		}
	}
}

void clear_aliases(alias_table_t *table)
{
	size_t i;
	for (i = 0; i < ALIAS_BUCKETS; i++)
	{
		alias_t *entry = table->alias_entries[i];
		while (entry != NULL)
		{
			alias_t *tmp = entry->next;
			free_command(entry->command);
			free(entry->command);
			free(entry->alias);
			free(entry);
			entry = tmp;
		}
	}
}

/*
	djb2 algorithm - see http://www.cse.yorku.ca/~oz/hash.html
*/
size_t hash(char *str)
{
	size_t hash_val = 5381;
	char c;

	while ((c = *str++))
	{
		hash_val = ((hash_val << 5) + hash_val) + c;
	}

	return hash_val;
}

size_t alias_hash(char *name)
{
	return hash(name) % ALIAS_BUCKETS;
}

