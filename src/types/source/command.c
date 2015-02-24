#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/command.h"

void print_single_command(command_t *command);

status_t copy_command(command_t *destination, command_t *source)
{
    destination->number = source->number;
    char **new_array = malloc(source->argc * sizeof *destination->arguments);
    if (new_array == NULL)
    {
        return MEMORY_ERROR;
    }

    size_t i;
    for (i = 0; i < source->argc - 1; i++)
    {
        new_array[i] = strdup(source->arguments[i]);
        if (new_array[i] == NULL)
        {
            size_t j;
            for (j = 0; j < i; j++)
            {
                free(new_array[j]);
            }
            free(new_array);
            return MEMORY_ERROR;
        }
    }
    new_array[i] = NULL;
	
	command_t *pipe = source->pipe;
	if (pipe != NULL)
	{
		command_t *pipe_copy = malloc(sizeof *pipe_copy);
		status_t error = copy_command(pipe_copy, pipe);
		if (error != SUCCESS)
		{
			size_t j;
			for (j = 0; j < i; j++)
			{
				free(new_array[j]);
			}
			free(new_array);
			free(pipe_copy);
			return error;
		}

		destination->pipe = pipe_copy;
	}
	else
	{
		destination->pipe = NULL;
	}

    destination->arguments = new_array;
    destination->argc = source->argc;
    destination->background = source->background;

    return SUCCESS;
}

void print_command(command_t *command)
{
	print_single_command(command);
	command_t *curr_command = command->pipe;
	while (curr_command != NULL)
	{
		fprintf(stdout, " | ");
		print_single_command(curr_command);
		curr_command = curr_command->pipe;
	}
}

void print_single_command(command_t *command)
{
    if (command->argc < 1)
    {
        return;
    }

    fprintf(stdout, "%s", command->arguments[0]);
    size_t j;
    for (j = 1; command->arguments[j]; j++)
    {
        fprintf(stdout, " %s", command->arguments[j]);
    }

    if (command->background)
    {
        fprintf(stdout, " &");
    }
}

void free_command(command_t *command)
{
    size_t i;
    for (i = 0; command->arguments[i]; i++)
    {
        free(command->arguments[i]);
    }
    free(command->arguments);

	if (command->pipe != NULL)
	{
		free_command(command->pipe);
		free(command->pipe);
	}
}
