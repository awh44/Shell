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

status_t copy_command(command_t *destination, command_t *source);
void print_command(command_t *command);
void free_command(command_t *command);

#endif
