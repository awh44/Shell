#include <stdlib.h>
#include <string.h>

#include "../include/parse.h"

/**
  * Trims, in place, the leading and trailing white space of the line with length length
  * @param line   the line to be trimmed
  * @param length the length of the line
  */
void trim(char *line, size_t length);

/**
  * Splits the given line on the character delim, not splitting on quotes if retain_quotes is true,
  * and returning an array of pointers INTO s. In other words, the function does not allocate new
  * strings for the elements of the returned array, and s is destroyed upon a call to this function.
  * @param s             the string to split; this string is not constant over this function
  * @param delim         the character on which to split
  * @param retain_quotes if true, if a delim is found within quotes, the func will not split on it
  * @param number        out param; indicates hwo many elements s split into
  * @return returns an array of elements split on delim; each element is NOT newly allocated but is
  * simply a pointer into s
  */
char **split(char *s, char delim, unsigned short retain_quotes, size_t *number);

status_t parse_line(char *line, size_t chars_read, command_t *command)
{
	//handle case of empty line
	if (chars_read <= 1)
	{
		return LINE_EMPTY;
	}

	//get rid of the newline by shortening the string by a character
	chars_read--;
	line[chars_read] = '\0';

	//get rid of leading and trailing whitespace
	trim(line, chars_read);

	//split the line into the arguments array
	command->arguments = split(line, ' ', 1, &command->argc);
	if (command->arguments == NULL)
	{
		return MEMORY_ERROR;
	}

	//determine if the process should be run in the background
	command->background = 0;
	if (strcmp(command->arguments[command->argc - 1], "&") == 0)
	{
		command->background = 1;
		command->arguments[command->argc - 1] = NULL;
	}
	else
	{
		command->argc++;
		char **tmp = realloc(command->arguments, command->argc * sizeof *tmp);
		if (tmp == NULL)
		{
			return MEMORY_ERROR;
		}
		
		command->arguments = tmp;
		command->arguments[command->argc - 1] = NULL;
	}

	return SUCCESS;
}

void trim(char *line, size_t length)
{
	size_t spaces = 0;
	while (line[spaces] == ' ')
	{
		spaces++;
	}

	size_t end = length - 1;
	if (spaces != 0)
	{
		memmove(line, line + spaces, length - spaces);
		end = length - spaces - 1;
		line[end + 1] = '\0';
	}

	while (line[end] == ' ')
	{
		end--;
	}

	line[end + 1] = '\0';
}

char **split(char *s, char delim, unsigned short retain_quotes, size_t *number)
{
	*number = 0;
    char **ret_val = NULL;
	unsigned short in_quotes = 0;
    char *start_pos = s;
    size_t i;
    for (i = 0; s[i]; i++)
    {
		if (s[i] == '"')
		{
			in_quotes = !in_quotes;
		}

		if ((s[i] == delim && !in_quotes) || (s[i + 1] == '\0'))
		{
			//reallocate space for another element in the array
			(*number)++;
			char **tmp = realloc(ret_val, *number * sizeof *ret_val);
			//if space can't be allocated, free the memory allocated so far and return NULL
			if (tmp == NULL)
			{
				free(ret_val);
				return NULL;
			}
			//otherwise, update the array's value
			ret_val = tmp;

			//next, indicate that the string starts at the given position. If it's the case where a
			//delimiter has been found, insert a null terminator to split the string up
			if (s[i] == delim)
			{
				s[i] = '\0';
			}
			ret_val[*number - 1] = start_pos;

			//update the starting position to be 1 after the delimiter	
			start_pos = s + i + 1;
        }
    }
    
	return ret_val;
}
