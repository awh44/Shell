#include <stdio.h>

#include "../include/history.h"

#define MIN(a, b) (a) < (b) ? (a) : (b)

status_t add_to_history(history_t *history, command_t *command)
{
	size_t index = history->num_commands % history->length;
	history->num_commands++;

	//save the old command's values, in case it needs to be freed later
	command_t old_command = history->commands[index];

	status_t error = copy_command(&history->commands[index], command);
	if (error != SUCCESS)
	{
		return error;
	}
	history->commands[index].number = history->num_commands;

	if (history->num_commands > history->length)
	{
		free_command(&old_command);
	}

	return SUCCESS;
}

void print_history(history_t *history)
{
	size_t num_to_do = MIN(history->num_commands, history->length);
	ssize_t start_index = history->num_commands % history->length;
	size_t i;
	for (i = 1; i <= num_to_do; i++)
	{
		ssize_t current_index = (start_index - (ssize_t) i) % (ssize_t) history->length;
		current_index += current_index < 0 ? history->length : 0;
		fprintf(stdout, "%zu ", history->commands[current_index].number);
		print_command(&history->commands[current_index]);
		fprintf(stdout, "\n");
	}		
}

void clear_history(history_t *history)
{
	size_t min = MIN(history->num_commands, history->length);
	size_t i;
	for (i = 0; i < min; i++)
	{
		free_command(&history->commands[i]);
	}
}

