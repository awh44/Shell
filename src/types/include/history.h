#ifndef __HISTORY__H__
#define __HISTORY__H__

#include <sys/types.h>
#include "command.h"

#define HISTORY_LENGTH 10
/**
  * A struct holding information about the command history, including an array of commands going a
  * set length back in history, the total number of commands executed, and the length of the
  * commands array
  */
typedef struct
{
	command_t commands[HISTORY_LENGTH];
	size_t num_commands;
	size_t length;
} history_t;

status_t add_to_history(history_t *history, command_t *command);
void print_history(history_t *history);
void clear_history(history_t *history);

#endif
