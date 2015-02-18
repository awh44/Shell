#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SUCCESS          0
#define READ_ERROR       1
#define LINE_EMPTY       2
#define MEMORY_ERROR     4
#define FORK_ERROR       8
#define EXEC_ERROR      16
#define NO_COMMANDS     32
#define NO_EXIST_ERROR  64
#define NUMBER_ERROR   128

#define HISTORY_LENGTH 10

#define ASCII_0 48
#define ASCII_9 57

#define MIN(a, b) (a) < (b) ? (a) : (b)

typedef int status_code;

typedef struct
{
	size_t number;
	char **arguments;
	size_t argc;
	unsigned short background;
} command_t;

typedef struct
{
	command_t commands[HISTORY_LENGTH];
	size_t num_commands;
} history_t;

unsigned short eval_print(char *line, size_t size, history_t *history);
status_code parse_line(char *line, size_t chars_read, command_t *command);
void trim(char *line, size_t length);
char **split(char *s, char delim, size_t *number);
status_code execute_builtin(history_t *history, command_t *command, unsigned short *is_builtin);
status_code execute(history_t *history, command_t *command);
void add_to_history(history_t *history, command_t *command);
void clear_history_entry(history_t *history, size_t index);
void clear_history(history_t *history);
void print_history(history_t *history);
void print_command(command_t *command);
void handle_error(status_code error_code);
status_code convert(char *s, size_t *value);

int main(int argc, char *argv[])
{
	int cont = 1;
	char *line = NULL;
	size_t size;
	history_t history = {0};

	while (cont)
	{
		fprintf(stdout, "osh> ");
		fflush(stdout);
		ssize_t chars_read = getline(&line, &size, stdin);
		if (chars_read < 0)
		{
			handle_error(READ_ERROR);
		}
		else
		{
			cont = eval_print(line, chars_read, &history);
		}
	}

	free(line);
	clear_history(&history);
	return 0;
}

unsigned short eval_print(char *line, size_t chars_read, history_t *history)
{
	if (strcmp(line, "exit\n") == 0)
	{
		return 0;
	} 

	command_t command = {0};
	status_code error = parse_line(line, chars_read, &command);
	if (error != SUCCESS)
	{
		handle_error(error);
		return 1;
	}

	unsigned short is_builtin;
	error = execute_builtin(history, &command, &is_builtin);
	if (!is_builtin)
	{
		error = execute(history, &command);
	}

	if (error == EXEC_ERROR)
	{
		//child process could not execute execvp, so free its memory and exit
		handle_error(error);
		free(command.arguments);
		free(line);
		clear_history(history);
		exit(1);
	}
	else if (error != SUCCESS)
	{
		handle_error(error);
	}


	//arguments must either be NULL or must not have been freed by one of the subsequently
	//called functions, so safe to just free it here.	
	free(command.arguments);
	return 1;
}

status_code parse_line(char *line, size_t chars_read, command_t *command)
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
	command->arguments = split(line, ' ', &command->argc);
	if (command->arguments == NULL)
	{
		return MEMORY_ERROR;
	}
	
	//determine if the process should be run in the background
	command->background = 0;
	if (strcmp(command->arguments[command->argc - 1], "&") == 0)
	{
		command->background = 1;
		(command->arguments)[command->argc - 1] = NULL;
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

char **split(char *s, char delim, size_t *number)
{
    char **ret_val = NULL;
    *number = 0;
    char *start_pos = s;
    size_t i;
    for (i = 0; s[i]; i++)
    {
        if (s[i] == delim || s[i + 1] == '\0')
        {
            (*number)++;
			char **tmp = realloc(ret_val, *number * sizeof *ret_val);
			if (tmp == NULL)
			{
				free(ret_val);
				return NULL;
			}

			ret_val = tmp;
            if (s[i] == delim)
			{
				s[i] = '\0';
			}
            ret_val[*number - 1] = start_pos;

            start_pos = s + i + 1;
        }
    }
    
	return ret_val;
}

status_code execute_builtin(history_t *history, command_t *command, unsigned short *is_builtin)
{
	if (strcmp(command->arguments[0], "history") == 0)
	{
		print_history(history);
		*is_builtin = 1;
		return SUCCESS;
	}

	if (command->arguments[0][0] == '!')
	{
		*is_builtin = 1;
		if (command->arguments[0][1] == '!')
		{
			if (history->num_commands >= 1)
			{
				command_t *old_command = &history->commands[(history->num_commands - 1) % HISTORY_LENGTH];
				print_command(old_command);
				return execute(history, old_command);
			}
			else
			{
				return NO_COMMANDS;
			}
		}
		else
		{
			size_t number;
			status_code error = convert(command->arguments[0] + 1, &number);
			if (error != SUCCESS)
			{
				return error;
			}
		
			ssize_t min_number = (ssize_t) history->num_commands - HISTORY_LENGTH + 1;
			if ((number > history->num_commands) || (ssize_t) number < min_number || number == 0)
			{
				return NO_EXIST_ERROR;
			}

			size_t index = (number - 1) & HISTORY_LENGTH;
			command_t *old_command = &history->commands[index];
			print_command(old_command);
			return execute(history, old_command);
		}
	}

	*is_builtin = 0;
	return SUCCESS;
}

status_code execute(history_t *history, command_t *command)
{
	add_to_history(history, command);
	pid_t pid = fork();
	if (pid < 0)
	{
		return FORK_ERROR;
	}
	else if (pid == 0)
	{
		//have child execute the desired program
		if (execvp(command->arguments[0], command->arguments) < 0)
		{
			return EXEC_ERROR;
		}
	}
	else if (!command->background)
	{
		int status;
		waitpid(pid, &status, NULL);
	}

	return SUCCESS;
}

void add_to_history(history_t *history, command_t *command)
{
	size_t index = history->num_commands % HISTORY_LENGTH;
	if (history->num_commands >= HISTORY_LENGTH)
	{
		clear_history_entry(history, index);
	}
	history->num_commands++;

	history->commands[index].number = history->num_commands;

	char **new_array = malloc(command->argc * sizeof *new_array);
	size_t i;
	for (i = 0; i < command->argc - 1; i++)
	{
		new_array[i] = strdup(command->arguments[i]);
	}
	new_array[i] = NULL;

	history->commands[index].arguments = new_array;
	history->commands[index].background = command->background;
	history->commands[index].argc = command->argc;
}

void clear_history_entry(history_t *history, size_t index)
{
	size_t i;
	for (i = 0; history->commands[index].arguments[i]; i++)
	{
		free(history->commands[index].arguments[i]);
	}
	free(history->commands[index].arguments);
}

void clear_history(history_t *history)
{
	size_t min = MIN(history->num_commands, HISTORY_LENGTH);
	size_t i;
	for (i = 0; i < min; i++)
	{
		clear_history_entry(history, i);
	}
}

void print_history(history_t *history)
{
	size_t num_to_do = history->num_commands < HISTORY_LENGTH ? history->num_commands : HISTORY_LENGTH;
	ssize_t start_index = history->num_commands % HISTORY_LENGTH;
	size_t i;
	for (i = 1; i <= num_to_do; i++)
	{
		ssize_t current_index = (start_index - (ssize_t) i) % HISTORY_LENGTH;
		current_index += current_index < 0 ? HISTORY_LENGTH : 0;
		print_command(&history->commands[current_index]);
	}		
}

void print_command(command_t *command)
{
	fprintf(stdout, "%zu", command->number);
	size_t j;
	for (j = 0; command->arguments[j]; j++)
	{
		fprintf(stdout, " %s", command->arguments[j]);
	}

	if (command->background)
	{
		fprintf(stdout, " &");
	}

	fprintf(stdout, "\n");
}


void handle_error(status_code error_code)
{
	switch (error_code)
	{
		case READ_ERROR:
			fprintf(stderr, "\nError: Could not read.");
			break;
		case LINE_EMPTY:
			break;
		case MEMORY_ERROR:
			fprintf(stderr, "Error: Could not allocate memory.");
			break;
		case FORK_ERROR:
			fprintf(stderr, "Error: Could not fork.");
			break;
		case EXEC_ERROR:
			fprintf(stderr, "Error: Could not execute the program.");
			break;
		case NO_COMMANDS:
			fprintf(stderr, "Error: No commands in history.");
			break;
		case NO_EXIST_ERROR:
			fprintf(stderr, "Error: That command does not exist in the history.");
			break;
		case NUMBER_ERROR:
			fprintf(stderr, "Error: Number formatted incorrectly.");
			break;
		default:
			fprintf(stderr, "Error: Unknown error.");
	}
	fprintf(stderr, "\n");
}

status_code convert(char *s, size_t *value)
{
	*value = 0;
	size_t length = strlen(s);
	size_t power10;
	size_t i;
	for (i = length - 1, power10 = 1; i < SIZE_MAX; i--, power10 *= 10)
	{
		if (s[i] < ASCII_0 || s[i] > ASCII_9)
		{
			return NUMBER_ERROR;
		}
		*value += (s[i] - ASCII_0) * power10;
	}

	return SUCCESS;
}
