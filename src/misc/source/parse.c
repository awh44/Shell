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

/**
  * Finds the given string str in the array arr with lenght length, returning the index if found and
  * -1 otherwise
  * @param arr    the array of strings to search
  * @param length the length of the array
  * @param str    the string to find in arr
  * @return the index of str in arr, if found, and -1 otherwise
  */
ssize_t find_str(char **arr, size_t length, char *str);

/**
  * Given an initially set up command, if that command has pipes, splits the command up and makes
  * command the head of a linked list of piped commands, maintained by the pipe pointers in the
  * struct
  * @param command the command to split up and make the head of the linked list
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t setup_pipes(command_t *command);

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
			free(command->arguments);
			return MEMORY_ERROR;
		}
		
		command->arguments = tmp;
		command->arguments[command->argc - 1] = NULL;
	}

	status_t error = setup_pipes(command);
	if (error != SUCCESS)
	{
		free(command->arguments);
		return error;
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

ssize_t find_str(char **arr, size_t length, char *str)
{
	ssize_t i;
	for (i = 0; i < length; i++)
	{
		if (strcmp(arr[i], str) == 0)
		{
			return i;
		}
	}

	return -1;
}

status_t setup_pipes(command_t *command)
{
	//find pipe. do the -1 to prevent find_str from looking at the NULL pointer
	ssize_t pipe_pos = find_str(command->arguments, command->argc - 1, "|");
	if (pipe_pos == 0)
	{
		return FORMAT_ERROR;
	}

	if (pipe_pos > 0)
	{
		size_t orig_argc = command->argc;
		command->argc = pipe_pos + 1;
		command->arguments[pipe_pos] = NULL;

		command_t *pipe_command = malloc(sizeof *pipe_command);
		if (pipe_command == NULL)
		{
			free(pipe_command);
			return MEMORY_ERROR;
		}

		pipe_command->arguments = command->arguments + pipe_pos + 1;
		pipe_command->argc = orig_argc - command->argc;
		pipe_command->background = command->background;
		status_t error = setup_pipes(pipe_command);
		if (error != SUCCESS)
		{
			free(pipe_command);
			return error;
		}

		command->pipe = pipe_command;
		return SUCCESS;
	}
	
	//base case - no more pipes
	command->pipe = NULL;
	return SUCCESS;
}
