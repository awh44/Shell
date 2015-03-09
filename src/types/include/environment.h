#ifndef __ENVIRONMENT__H__
#define __ENVIRONMENT__H__

#include "alias.h"
#include "history.h"
#include "path.h"

/**
  * Holds all of the information about the user's current environment, including their path
  * variable, their history, their aliases, and any open script file, with plenty room for any more
  * to come
  */
typedef struct
{
	path_t *path;
	history_t *history;
	alias_table_t *aliases;
	int script_file;
	unsigned short verbose;
	string_t *prompt;
} environment_t;

/**
  * Clears and frees the memory and open information associated with the environment, including
  * clearing the path, history, and aliases
  * @param table the table to be cleared
  */
void clear_environment(environment_t *environment);

#endif
