#ifndef __PARSE__H__
#define __PARSE__H__

#include <stddef.h>

#include "../../types/include/command.h"
#include "../../types/include/status.h"

/**
  * Parses the given line made up of chars_read characters, setting the values of command in doing
  * so. Note that this function does NOT allocate new memory for each element of the command's
  * arguments array. That is, each element of arguments points at a part of the pre-existing line.
  * the arguments array is also NULL terminated, and argc counts the NULL pointer as an argument.
  * @param line       the line to be parsed
  * @param chars_read the number of characters read from the command line (i.e., the length of line)
  * @param command    out param; arguments is NULL terminated and elements set to diff. pos. of line
  * @return a status code indicating whether an error occurred during execution of the function
 */
status_t parse_line(char *line, size_t chars_read, command_t *command);

/**
  * Given an initially set up command, if that command has pipes, splits the command up and makes
  * command the head of a linked list of piped commands, maintained by the pipe pointers in the
  * struct. Note that this function is similar to split. After executing it, the linked list of
  * commands' argument pointers will ALL be pointing at command's original array. So in other words,
  * after a call to this function, free(command->pipe->arguments) should NEVER be called, and
  * free_command should NEVER be called on the command passed to this function or any of the linked
  * list elements. However, the elements of the linked list do still need to be freed.
  * @param command the command to split up and make the head of the linked list
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t setup_pipes(command_t *command);


#endif
