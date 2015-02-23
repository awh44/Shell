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

/**
  * Adds a command entry to the given history (performing a deep copy of command to do so)
  * @param history   the history into which the new entry should be placed
  * @param command   the command to be added to the history 
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t add_to_history(history_t *history, command_t *command);

/**
  * Prints the history associated with the given variable
  * @param history the history taht is to be printed
  */
void print_history(history_t *history);

/**
  * Clears and frees all of the memory associated with the history
  * @param history the history to be cleared
  */
void clear_history(history_t *history);

#endif
