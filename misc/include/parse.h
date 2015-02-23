#ifndef __PARSE__H__
#define __PARSE__H__

#include <stddef.h>

#include "../../types/include/status.h"
#include "../../types/include/command.h"

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

#endif
