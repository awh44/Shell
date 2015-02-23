#ifndef __ENVIRONMENT__H__
#define __ENVIRONMENT__H__

#include "path.h"
#include "history.h"
#include "alias.h"

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
} environment_t;

void clear_environment(environment_t *environment);
#endif
