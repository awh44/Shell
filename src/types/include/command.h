#ifndef __COMMAND__H__
#define __COMMAND__H__

#include <stddef.h>

#include "status.h"

/**
  * A struct holding information about a command, including its command number, the arguments, the
  * number of arguments, and whether it is to execute in the background.
  */
typedef struct
{
    size_t number;
    char **arguments;
    size_t argc;
    unsigned short background;
} command_t;

/**
  * Performs a deep copy of the command pointed to by source into the command pointed to by
  * destination
  * @param destination where the new copy will be placed
  * @param source      the command to copy
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t copy_command(command_t *destination, command_t *source);

/**
  * Prints the command with its arguments, all on one line, without a newline
  * @param command the command to be printed
  */
void print_command(command_t *command);

/**
  * Frees all of the memory associated wtih the given command
  * @param command the command to be freed
  */
void free_command(command_t *command);

#endif
